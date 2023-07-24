// See https://aka.ms/new-console-template for more information
using System.IO.Ports;

Console.WriteLine("Hello, World!");
 Console.WriteLine("Available Ports:");
        foreach (string s in SerialPort.GetPortNames())
        {
            Console.WriteLine(s);
        }

        Console.WriteLine("done...");