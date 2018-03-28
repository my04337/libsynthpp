#include <LSP/Base/StackTrace.hpp>

#include <sstream>

// プラットフォーム依存 : デバッグ関連情報
#if defined(WIN32)
#   include <Windows.h>
#   include <winnt.h>
#   include <winver.h>
#   include <DbgHelp.h>
#   include <Psapi.h>
#   include <commdlg.h>
#   include <WCHAR.h>
#   pragma warning(disable:4826)
#   pragma comment(lib, "dbghelp.lib")
#   pragma comment(lib, "version.lib")
#   pragma comment(lib, "Comdlg32.lib")
#else
#   include <execinfo.h>  // for backtrace
#   include <dlfcn.h>     // for dladdr
#   include <cxxabi.h>    // for __cxa_demangle
#endif

using namespace LSP;


namespace {

thread_local void* STACK_BOTTOM_ADDR = nullptr;

#if defined(WIN32)

// ---

struct ModuleInfo : non_copy
{
	struct Detail {
		std::filesystem::path modulePath = "???";
		LPVOID baseOfDll    = NULL;
		DWORD  sizeOfImage  = 0;
		LPVOID entryPoint   = NULL;
		std::string version = "";
	};

	static ModuleInfo gather() {
		ModuleInfo ret;
		HMODULE hMods[1024]; // ロードされるDLLは高々百数十個と過程
		DWORD processID = GetCurrentProcessId();
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, processID );
		if(NULL == hProcess) return ret;

		DWORD cbNeeded;
		if(!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) return ret;

		for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ ) {
			ModuleInfo::Detail info;

			// module name
			WCHAR szModPath[MAX_PATH];
			if(GetModuleFileNameExW(hProcess, hMods[i], szModPath, sizeof(szModPath)/sizeof(WCHAR))) {
				info.modulePath = szModPath;

				// module version
				DWORD cbBlock = GetFileVersionInfoSizeW(szModPath, 0);
				if(cbBlock) {
					BYTE *pbBlock = (BYTE *)malloc(cbBlock);
					if(GetFileVersionInfoW(szModPath, 0, cbBlock, (LPVOID)pbBlock)) {
						LPVOID pvBuf = NULL;
						UINT uLen = 0;
						if(VerQueryValueW((const LPVOID)pbBlock, L"\\", &pvBuf, &uLen)) {
							VS_FIXEDFILEINFO *pvfi = (VS_FIXEDFILEINFO *)pvBuf;
							DWORD dwFileVerMS = pvfi->dwFileVersionMS;
							DWORD dwFileVerLS = pvfi->dwFileVersionLS;
							CHAR buf[256];
							sprintf_s(buf, sizeof(buf)/sizeof(buf[0]), "%d.%d.%d.%d", HIWORD(dwFileVerMS), LOWORD(dwFileVerMS), HIWORD(dwFileVerLS), LOWORD(dwFileVerLS));
							info.version = buf;

						}
					}
					free(pbBlock);
				}
			}

			// module image
			MODULEINFO modInfo;
			if(GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo))) {
				info.baseOfDll   = modInfo.lpBaseOfDll;
				info.sizeOfImage = modInfo.SizeOfImage;
				info.entryPoint  = modInfo.EntryPoint;
			}

			ret.details.push_back(info);
		}

		CloseHandle( hProcess );

		return ret;
	}


	const Detail& hitTest(LPVOID codePoint) {
		static const Detail empty_detail;
		for(auto& d : details) {
			if((size_t)codePoint >= (size_t)d.baseOfDll && (size_t)codePoint < (size_t)d.baseOfDll+d.sizeOfImage) {
				return d;
			}
		}
		return empty_detail;
	}

	// ---
	std::list<Detail> details;
};
void initializeDebugSymbols()
{
	static std::once_flag once;
	std::call_once(once, [](){
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		SymInitialize(GetCurrentProcess(), NULL, TRUE);
	});
}

#ifdef _M_X64
STACKFRAME64 initializeStackFrame(const CONTEXT& c)
{
	STACKFRAME64 s;
	s.AddrPC.Offset = c.Rip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrStack.Offset = c.Rsp;
	s.AddrStack.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Rbp;
	s.AddrFrame.Mode = AddrModeFlat;
	return s;
}
#else
STACKFRAME64 initializeStackFrame(const CONTEXT& c) {
	STACKFRAME64 s;
	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrStack.Offset = c.Esp;
	s.AddrStack.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;
	return s;
}
#endif
#endif
}

// スタックトレースの出力
CppCallStack::StackTrace CppCallStack::getStackTrace(size_t skipFrames)noexcept
{
	StackTrace stacks;

	// 最大でも256スタックまでに制限する (必要メモリ制限)
#if defined(WIN32)
	initializeDebugSymbols();

	stacks.resize(256, nullptr);
	USHORT stackNum = CaptureStackBackTrace(static_cast<DWORD>(skipFrames), static_cast<DWORD>(stacks.size()), &stacks[0], NULL);
	stacks.resize(stackNum, nullptr);
#else
	stacks.resize(256);
	int real_stacks = backtrace(&stacks[0], stacks.size());
	stacks.resize(real_stacks);
#endif

	return stacks; // NRVO
}
CppCallStack::StackTrace CppCallStack::getStackTraceSEH(void* hThread_, void* pContextRecord)noexcept
{
	StackTrace stacks;
#if defined(WIN32)
	initializeDebugSymbols();

	HANDLE process = GetCurrentProcess();
	HANDLE hThread = reinterpret_cast<HANDLE>(hThread_);
	CONTEXT& c = *reinterpret_cast<CONTEXT*>(pContextRecord);
	STACKFRAME64 s = initializeStackFrame(c);

#ifdef _M_X64
	DWORD image_type = IMAGE_FILE_MACHINE_AMD64;
#else
	DWORD image_type = IMAGE_FILE_MACHINE_I386;
#endif
	do {
		if (!StackWalk64(image_type, process, hThread, &s, &c, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
			break;
		}
		stacks.push_back(reinterpret_cast<void*>(s.AddrPC.Offset));
	} while (s.AddrReturn.Offset != 0);

#else
	lsp_assert_desc(false, "for win32 platform");
#endif
	return stacks; // NRVO
}

// スタックサイズの取得
size_t CppCallStack::getStackSize()noexcept
{
	int dummy_variable; // この変数の座標を現在の大まかなスタック位置として扱う
	if(STACK_BOTTOM_ADDR == 0) return 0;

	return static_cast<size_t>(std::abs(reinterpret_cast<int64_t>(&dummy_variable) - reinterpret_cast<int64_t>(STACK_BOTTOM_ADDR)));
}
void CppCallStack::setStackBottom()noexcept
{
	int dummy_variable; // この変数の座標を現在の大まかなスタック位置として扱う
	STACK_BOTTOM_ADDR = &dummy_variable;
}

void CppCallStack::printStackTrace(std::ostream& stream, const StackTrace& stacks, size_t max_stack_num)noexcept
{
	stream << "Stack Trace :" << std::endl;
	const auto stackNum = std::min(stacks.size(), max_stack_num);

#if defined(WIN32)
	initializeDebugSymbols();

	// スタックトレースの取得
	auto info = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO)+256*sizeof(WCHAR));
	auto fin_act_release_info = [info]{ free(info); };

	memset(info, 0, sizeof(SYMBOL_INFO)+256*sizeof(WCHAR));
	info->MaxNameLen   = 255;
	info->SizeOfStruct = sizeof( SYMBOL_INFO );

	ModuleInfo modInfo = ModuleInfo::gather();

	const HANDLE hProcess = GetCurrentProcess();
	bool hasMeaningfulStack = false;
	for(size_t i=0; i<stackNum; ++i) {
		constexpr int ptrw = sizeof(std::nullptr_t)*8/4;

		void* pc = stacks[i];
		auto pc64 = static_cast<DWORD64>(reinterpret_cast<size_t>(pc));
		const ModuleInfo::Detail& d = modInfo.hitTest(pc);

		// 関数名
		LPCSTR name = "?";
		if(SymFromAddr(hProcess, pc64, 0, &*info)) {
			name = info->Name;
		}

		// 特定の関数(特にstd::function絡み)は長ったらしいため省略する
		if(strstr(name, "std::invoke") == name) continue;
		if(strstr(name, "std::_Func_") == name) continue;
		if(strstr(name, "std::_Invoke_") == name) continue;
		if(strstr(name, "std::_Invoker_") == name) continue;
		// ログ出力関係も代わり映えがないため除外
		if(strstr(name, "LSP::Log::") == name) continue;
		if(strstr(name, "LSP::CppCallStack::") == name) continue;
		// 処理系で予約されている名前も省略
		if(name[0] == '_' && ('A' <= name[1] && name[1] <= 'Z')) continue;
		// その他Win32Apiなどで頻出の名前も除外
		if(strstr(name, "Rtl") == name) continue;
		// 不明なスタックトレースは除外(先頭部分のみ)
		if(!hasMeaningfulStack && d.modulePath.filename() ==  "???" && strcmp(name, "?")==0) {
			continue;
		}
		hasMeaningfulStack = true;

		// line & module info
		bool hasLineInfo = false;
		IMAGEHLP_LINE64 line;
		DWORD lineDisp = 0;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		if(SymGetLineFromAddr64(hProcess, pc64, &lineDisp, &line)) {
			hasLineInfo = true;
		}

		// num
		stream << stackNum-i-1 << " : ";

		// name
		stream << name;
		stream << " -- ";

		// line & module info
		if(hasLineInfo) {
			std::filesystem::path srcFileName = line.FileName;
			stream << srcFileName.filename() << ":" << line.LineNumber;
			stream << " (" << d.modulePath.filename() << ")";
		} else {
			stream << d.modulePath.filename();
		}

		stream << std::endl; // end

						// main関数以降は出力しない
		if(strstr(name, "main") == name) break;
	}
#else
	char** syms = backtrace_symbols(&stacks[0], stackNum);
	for(size_t i=0; i<stackNum; ++i) {
		Dl_info info;
		if (dladdr(stacks[i], &info)) {
			// num
			stream << stackNum-i-1 << " : ";

			// name
			stream << demangle(info.dli_sname);
			stream << " -- ";

			// line & module info
			stream << QFileInfo(strings::from(info.dli_fname)).fileName() << u"+" << (size_t)info.dli_fbase;

			stream << std::endl;
		} else {
			stream << QString::fromLocal8Bit(syms[i]) << std::endl;
		}
	}
	free(syms);

#endif
	stream << "Stack Trace End" << std::endl;
}
