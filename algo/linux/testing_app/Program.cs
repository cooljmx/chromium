using System;
using System.Net.NetworkInformation;
using System.IO;
using System.IO.Pipes;
using System.Reflection;
using System.Text;
using System.Threading;

namespace testing_app
{
    static class Program
    {
        public static int DisplayNetworkConfiguration(IntPtr arg, int argLength)
        {
            NetworkInterface[] adapters = NetworkInterface.GetAllNetworkInterfaces();
            foreach (NetworkInterface adapter in adapters)
            {
                IPInterfaceProperties properties = adapter.GetIPProperties();
                var path = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                Console.WriteLine(path);
                Console.WriteLine(adapter.Description);
            }
            Console.WriteLine();
            return 2;
        }

        public static int HelloWorldFromDotNetCore(IntPtr arg, int argLength)
        {
            Console.WriteLine("Hello World from .Net");
            Console.WriteLine();
            return 0;
        }

        //  use arg
        public static int ReverseLine(IntPtr arg, int argLength)
        {
            Console.WriteLine("Write to pipe in_pipe and read from out_pipe");
            return 0;
        }

        static void Main(string[] args)
        {
            Console.WriteLine("The display name is ");
            Console.WriteLine(typeof(Program).Assembly.FullName);

            Console.WriteLine("Qualified name is ");
            Console.WriteLine(typeof(Program).AssemblyQualifiedName);

            DisplayNetworkConfiguration(IntPtr.Zero, 0);

            Thread.Sleep(3000);
        }
    }
}
