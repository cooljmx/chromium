namespace cTrader.Automate.Host.NetCore;

public static class Endpoint
{
    [STAThread]
    public static int Run(IntPtr arg, int argLength)
    {
        var commandLineArgs = Environment.GetCommandLineArgs();

        Console.WriteLine($"[Target process] Started with {commandLineArgs.Length} command line arguments:");

        foreach (var args in commandLineArgs)
            Console.WriteLine($"\t\"{args}\"");

        Console.WriteLine("[Target process] Press any key to finish...");
        Console.ReadKey();

        return 0;
    }
}