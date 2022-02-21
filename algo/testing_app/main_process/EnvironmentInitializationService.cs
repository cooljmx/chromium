using System.Reflection;
using System.Runtime.InteropServices;

namespace main_process;

public static class EnvironmentInitializationService
{
    public static void Initialize()
    {
        var entryAssemblyLocation = Assembly.GetEntryAssembly()?.Location;
        var productDirectory = entryAssemblyLocation != null ? Path.GetDirectoryName(entryAssemblyLocation) : null;

        if (string.IsNullOrWhiteSpace(productDirectory))
            throw new InvalidOperationException();

        var hostFrxPath = Path.Combine(productDirectory, GetHostFxrRelativePath());
        var runtimePath = GetRuntimePath();

        Environment.SetEnvironmentVariable(AppHostConstants.CT_ENV_VAR_PRODUCT_PATH, productDirectory);
        Environment.SetEnvironmentVariable(AppHostConstants.CT_ENV_VAR_HOSTFXR_PATH, hostFrxPath);
        Environment.SetEnvironmentVariable(AppHostConstants.CT_ENV_VAR_DOTNET_PATH, runtimePath);
    }

    private static string GetHostFxrRelativePath()
    {
        return Environment.Is64BitProcess
#if OS_WINDOWS
            ? "x64\\hostfxr.dll"
#elif OS_LINUX
            ? "x64/hostfxr.dll"
#endif

#if OS_WINDOWS
            : "x86\\hostfxr.dll";
#elif OS_LINUX
            :  throw new InvalidOperationException("x86 system is not supported");
#endif
    }

    private static string GetRuntimePath()
    {
        var runtimeVersionPath = Path.TrimEndingDirectorySeparator(RuntimeEnvironment.GetRuntimeDirectory());

        var runtimeTypePath = Path.GetDirectoryName(runtimeVersionPath)
            .NotNull("Path.GetDirectoryName(runtimeVersionPath) != null");

        var sharedPath = Path.GetDirectoryName(runtimeTypePath)
            .NotNull("Path.GetDirectoryName(runtimeTypePath) != null");

        return Path.GetDirectoryName(sharedPath)
            .NotNull("Path.GetDirectoryName(sharedPath) != null");
    }

    public static T NotNull<T>(this T? instanceOrNull, string? name = null)
        where T : class
    {
        if (instanceOrNull == null)
            throw new NullReferenceException($"reference {name ?? ""} is null but must not be");

        return instanceOrNull;
    }
}