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

        public static int ReverseLine(IntPtr arg, int argLength)
        {
			bool windows = System.OperatingSystem.IsWindows();
			if (windows)
			{
				var hello = "Hello world from dotnet on Windows!!!";
				Console.WriteLine(hello);

				var doc_folder = Environment.GetFolderPath(Environment.SpecialFolder.Personal);
				var out_path = Path.Combine(doc_folder, "out.txt");
				var out_file = new FileInfo(out_path);
                FileStream writable_stream2 = out_file.OpenWrite();
                var writer2 = new StreamWriter(writable_stream2, Encoding.ASCII);
                writer2.Write(hello);
                writer2.Flush();
                writable_stream2.Close();
				return 0;
			}
            Console.WriteLine("Now please open a new shell and run ./algo/linux/pipes_test.sh");
            Console.WriteLine();

            Span<char> to_be_reversed = stackalloc char[2 << 12];
            var in_pipe = new FileInfo("in_pipe");
            var out_pipe = new FileInfo("out_pipe");

            while (true) {
                FileStream readable_stream = in_pipe.OpenRead();
                FileStream writable_stream = out_pipe.OpenWrite();
                var reader = new StreamReader(readable_stream, Encoding.ASCII);
                var writer = new StreamWriter(writable_stream, Encoding.ASCII);

                reader.Read(to_be_reversed);

                to_be_reversed.Reverse();

                writer.Write(to_be_reversed);
                writer.Flush();

                to_be_reversed.Clear();
                readable_stream.Close();
                writable_stream.Close();
            }
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
