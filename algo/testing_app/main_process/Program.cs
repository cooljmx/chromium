using System.Diagnostics;
using System.Reflection;
using main_process;

EnvironmentInitializationService.Initialize();

var executingAssembly = Assembly.GetExecutingAssembly();

var executingAssemblyLocation = Path.GetDirectoryName(executingAssembly.Location);

var algoHostPath = Path.Combine(
    executingAssemblyLocation,
    "algohost.netcore",
#if OS_WINDOWS
    Environment.Is64BitProcess ? "x64" : "x86",
    "algohost.netcore.exe"
#elif OS_LINUX
    "linux-x64",
    "algohost.netcore"
#endif
    );

var id = Guid.NewGuid().ToString("N");

Console.WriteLine($"Starting child process with {id}");

var processStartInfo = new ProcessStartInfo
{
    FileName = algoHostPath,
    Arguments = id,
    CreateNoWindow = true,
    UseShellExecute = true,
    WindowStyle = ProcessWindowStyle.Normal
};

var process = Process.Start(processStartInfo) ?? throw new InvalidOperationException();

process.WaitForExit();

Console.WriteLine($"Process exit code: {process.ExitCode}");