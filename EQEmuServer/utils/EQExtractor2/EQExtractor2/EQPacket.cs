﻿//
// Copyright (C) 2001-2010 EQEMu Development Team (http://eqemulator.net). Distributed under GPL version 2.
//
// 
// This is the netcode for turning SOE protocol packets into EQ Application Packets
//
using System;
using System.Text;
using System.IO;
using System.Collections.Generic;
using zlib;
using MyUtils;
using EQExtractor2.Patches;
using EQExtractor2.OpCodes;
using EQApplicationLayer;

namespace EQPacket
{
    public enum PacketDirection { Unknown, ClientToServer, ServerToClient };

    public class EQApplicationPacket
    {
        public EQApplicationPacket(System.Net.IPAddress srcIp, System.Net.IPAddress dstIp, ushort srcPort, ushort dstPort, int OpCode, int BufferSize, byte[] Source, int Offset, PacketDirection Direction, DateTime PacketTime)
        {
            this.OpCode = OpCode;

            this.Direction = Direction;

            this.PacketTime = PacketTime;

            if (BufferSize < 0)
            {
                //Log("OPCode " + OpCode.ToString("x") + ", Buffer Size " + BufferSize + " Buffer Size < 0 !");
                return;
            }

            Buffer = new byte[BufferSize];

            Array.Copy(Source, Offset, Buffer, 0, BufferSize);
        }

        public byte[] Buffer;
        public int OpCode;
        public PacketDirection Direction;
        public DateTime PacketTime;
        public bool Locked = false;
    }
    
    public class PacketManager
    {
        const Byte DEBUG = 0;
    	const Byte DUMPPACKETS = 0;

	    private void Debug(string Message)
        {
            #pragma warning disable 0162

            if (DEBUG == 1)
    	        Log(Message);

            #pragma warning restore 0162
        }

        public bool ErrorsInStream = false;
        	
    	const UInt32 OP_SessionRequest = 0x0001;
    	const UInt32 OP_SessionResponse = 0x0002;
    	const UInt32 OP_Combined = 0x0003;
    	const UInt32 OP_Packet = 0x0009;
    	const UInt32 OP_Fragment = 0x000d;
    	const UInt32 OP_OutOfOrderAck = 0x0011;
    	const UInt32 OP_Ack = 0x0015;

        private bool PermaLocked = false;
        private bool Identified = false;

        private int FragmentSeq = -1;
        private int FragmentedPacketSize = 0;
        private int FragmentedBytesCollected = 0;
        private byte[] Fragments;

        private System.Net.IPAddress ServerIP;
        private ushort ServerPort = 0;
        private int ExpectedServerSEQ = 0;

        private System.Net.IPAddress ClientIP;
        private ushort ClientPort = 0;
        private int ExpectedClientSEQ = 0;

        StreamIdentifier Identifier = null;
        
        public List<EQApplicationPacket> PacketList = new List<EQApplicationPacket>();

	    LogHandler Logger = null;

	    public void SetLogHandler(LogHandler Logger)
	    {
    		this.Logger = Logger;
    	}

    	void Log(string Message)
	    {
    		if(Logger != null)
			    Logger(Message);
	    }

        public delegate IdentificationStatus StreamIdentifier(int OPCode, int Size, PacketDirection Direction);

        public void SetVersionIdentifierMethod(StreamIdentifier Identifier)
        {
            this.Identifier = Identifier;
        }

        public void AddPacket(EQApplicationPacket Packet)
        {
            #pragma warning disable 0162

            if (DUMPPACKETS > 0)
	        {
                Log("[OPCode: 0x" + Packet.OpCode.ToString("x") + "] [Size: " + Packet.Buffer.Length + "]");
	            Log(Utils.HexDump(Packet.Buffer));
            }

            #pragma warning restore 0162

            Packet.Locked = PermaLocked;

            PacketList.Add(Packet);
        }
        
        private struct CacheEntry
        {
            public int Seq;
            public PacketDirection Direction;
            public byte[] Payload;
            public DateTime PacketTime;
            public bool SubPacket;

            public CacheEntry(int inSeq, PacketDirection inDirection, byte[] inPayload, DateTime inPacketTime, bool inSubPacket)
            {
                Seq = inSeq;
                Direction = inDirection;
                Payload = inPayload;
                PacketTime = inPacketTime;
                SubPacket = inSubPacket;
            }            
        }

        private List<CacheEntry> Cache = new List<CacheEntry>();

        private void AddToCache(int Seq, PacketDirection Direction, byte[] Payload, DateTime PacketTime, bool SubPacket)
        {
	        Debug("Adding packet with Seq " + Seq + " to cache.");
            foreach(CacheEntry Existing in Cache)
            {
                if((Existing.Direction == Direction) && (Existing.Seq == Seq))
                    return;
            }
            CacheEntry ce = new CacheEntry(Seq, Direction, Payload, PacketTime, SubPacket);

            Cache.Add(ce);
        }

        private bool CacheEmpty()
        {
            return (Cache.Count == 0);
        }

        public void DumpCache()
        {
            Log("Cache has " + Cache.Count + " elements.");
            for (int i = 0; i < Cache.Count; ++i)
            {
                string Direction;

                if (Cache[i].Direction == PacketDirection.ClientToServer)
                    Direction = "Client to Server";
                else if (Cache[i].Direction == PacketDirection.ServerToClient)
                    Direction = "Server to Client";
                else
                    Direction = "Unknown";

                Log("Cache Entry Seq " + Cache[i].Seq + ", Direction:" + Direction);
            }
        }

        private int FindCacheEntry(int Seq, PacketDirection Direction)
        {
            for(int i = 0; i < Cache.Count; ++i)
            {
                CacheEntry ce = Cache[i];

                if ((ce.Seq == Seq) && (ce.Direction == Direction))
                    return i;
            }
            return -1;
        }

        private void ProcessCache()
        {
            int CacheElement;

            CacheElement = FindCacheEntry(GetExpectedSeq(PacketDirection.ServerToClient), PacketDirection.ServerToClient);
            
            while ( CacheElement >= 0)
            {
                ProcessPacket(ServerIP, ClientIP, ServerPort, ClientPort, Cache[CacheElement].Payload, Cache[CacheElement].PacketTime, Cache[CacheElement].SubPacket, true);

                Cache.RemoveRange(CacheElement, 1);

                CacheElement = FindCacheEntry(GetExpectedSeq(PacketDirection.ServerToClient), PacketDirection.ServerToClient);
            }

            CacheElement = FindCacheEntry(GetExpectedSeq(PacketDirection.ClientToServer), PacketDirection.ClientToServer);

            while (CacheElement >= 0)
            {
                ProcessPacket(ClientIP, ServerIP, ClientPort, ServerPort, Cache[CacheElement].Payload, Cache[CacheElement].PacketTime, Cache[CacheElement].SubPacket, true);

                Cache.RemoveRange(CacheElement, 1);

                CacheElement = FindCacheEntry(GetExpectedSeq(PacketDirection.ServerToClient), PacketDirection.ServerToClient);
            }
        }

        private PacketDirection GetDirection(System.Net.IPAddress srcIp, System.Net.IPAddress dstIp, ushort srcPort, ushort dstPort)
        {
            if ((srcIp.Equals(ServerIP)) && (srcPort == ServerPort) && (dstIp.Equals(ClientIP)) && (dstPort == ClientPort))
                return PacketDirection.ServerToClient;

            if ((srcIp.Equals(ClientIP)) && (srcPort == ClientPort) && (dstIp.Equals(ServerIP)) && (dstPort == ServerPort))
                return PacketDirection.ClientToServer;

            return PacketDirection.Unknown;
        }

        private int GetExpectedSeq(PacketDirection Direction)
        {
            if (Direction == PacketDirection.ClientToServer)
                return ExpectedClientSEQ;
            else if (Direction == PacketDirection.ServerToClient)
                return ExpectedServerSEQ;

            return 0;
        }

        private void AdvanceSeq(PacketDirection Direction)
        {
            if (Direction == PacketDirection.ClientToServer)
                ++ExpectedClientSEQ;
            else if (Direction == PacketDirection.ServerToClient)
                ++ExpectedServerSEQ;
        } 
        
        public void ProcessPacket(System.Net.IPAddress srcIp, System.Net.IPAddress dstIp, ushort srcPort, ushort dstPort, byte[] Payload, DateTime PacketTime, bool SubPacket, bool Cached)
        {
            UInt32 OpCode = (UInt32)(Payload[0] * 256 + Payload[1]);

            PacketDirection Direction = GetDirection(srcIp, dstIp, srcPort, dstPort);

            if ((Direction == PacketDirection.Unknown) && (OpCode != OP_SessionRequest))
                return;

            switch (OpCode)
            {
                case OP_SessionRequest:
		{
                    if (PermaLocked)
                        break;

                    ClientIP = srcIp;

                    ClientPort = srcPort;

                    ServerIP = dstIp;

                    ServerPort = dstPort;
                    
        		    Log("-- Locked onto EQ Stream. Client IP " + srcIp + ":" + srcPort + " Server IP " + dstIp + ":" + dstPort);

                    ExpectedClientSEQ = 0;

                    ExpectedServerSEQ = 0;

                    break;
                }

                case OP_SessionResponse:
                    break;

                case OP_Combined:
                {
		            Debug("OP_Combined");
                    //Debug(Utils.HexDump(Payload));

                    byte[] Uncompressed;

                    if (!SubPacket && (Payload[2] == 0x5a))
                    {
			            Uncompressed = DecompressPacket(Payload, 3);
                    }
                    else if (!SubPacket && (Payload[2] == 0xa5))
                    {
                        Uncompressed = new byte[Payload.Length - 3];

                        Array.Copy(Payload, 3, Uncompressed, 0, Payload.Length - 3);

                    }
                    else
                    {
                        Uncompressed = new byte[Payload.Length - 2];

                        Array.Copy(Payload, 2, Uncompressed, 0, Payload.Length - 2);
                    }

                    int BufferPosition = 0;

                    while (BufferPosition < Uncompressed.Length - 2)
                    {
                        int SubPacketSize = Uncompressed[BufferPosition++];

                        byte[] NewPacket = new byte[SubPacketSize];

                        Array.Copy(Uncompressed, BufferPosition, NewPacket, 0, SubPacketSize);

			            BufferPosition += SubPacketSize;

                        ProcessPacket(srcIp, dstIp, srcPort, dstPort, NewPacket, PacketTime, true, Cached);
                    }
                    break;
                }

                case OP_Packet:
                {
		            Debug("OP_Packet, Subpacket = "+ SubPacket);

                    byte[] Uncompressed;

                    if (!SubPacket && (Payload[2] == 0x5a))
                    {
                        Debug("Compressed");
			            Uncompressed = DecompressPacket(Payload, 3);
                    }
                    else if (!SubPacket && (Payload[2] == 0xa5))
                    {
                        Debug("0xa5");
                 
                        Uncompressed = new byte[Payload.Length - 5];

			            Array.Copy(Payload, 3, Uncompressed, 0, Payload.Length - 5);
                    }
                    else
                    {
                        Debug("Uncompressed");
                        Uncompressed = new byte[Payload.Length - 2];

			            Array.Copy(Payload, 2, Uncompressed, 0, Payload.Length - 2);
                    }
                    Debug("Raw payload is:");
                    //Debug(Utils.HexDump(Payload));
                    Debug("Uncompressed data is:");
                    //Debug(Utils.HexDump(Uncompressed));

                    int Seq = Uncompressed[0] * 256 + Uncompressed[1];

                    if (Seq != GetExpectedSeq(Direction))
                    {
                        if (Seq > GetExpectedSeq(Direction))
                        {
                            if ((Seq - GetExpectedSeq(Direction) < 1000))
                                AddToCache(Seq, Direction, Payload, PacketTime, SubPacket);
                            else
                            {
			                    Log("Giving up on seeing expected fragment.");

                                ErrorsInStream = true;

			                    FragmentSeq = -1;

                                AdvanceSeq(Direction);
                            }
                        }
                        
                        break;
                    }
                    else
                        AdvanceSeq(Direction);
                    

                    bool Multi = ((Uncompressed[2] == 0x00) && (Uncompressed[3] == 0x19));

                    if (Multi)
                    {
                        int BufferPosition = 4;

                        while (BufferPosition < (Uncompressed.Length - 2))
                        {
                            int Size = 0;

                            if ((Uncompressed[BufferPosition] == 0xff) && (Uncompressed[BufferPosition + 1] == 0x01))
                            {

                                Size = 256 + Uncompressed[BufferPosition + 2];
                                BufferPosition += 3;
                            }
                            else
                                Size = Uncompressed[BufferPosition++];

                            int AppOpCode = Uncompressed[BufferPosition++] + (Uncompressed[BufferPosition++] * 256);
                            
                            ProcessAppPacket(srcIp, dstIp, srcPort, dstPort, AppOpCode, Size - 2, Uncompressed, BufferPosition, Direction, PacketTime);
                            
                            BufferPosition = BufferPosition + (Size - 2);
                        }
                    }
                    else
                    {
                        int BufferPosition = 2;

                        int AppOpCode = Uncompressed[BufferPosition++] + (Uncompressed[BufferPosition++] * 256);                     

                        ProcessAppPacket(srcIp, dstIp, srcPort, dstPort, AppOpCode, Uncompressed.Length - 4, Uncompressed, BufferPosition, Direction, PacketTime);
                    }
                    break;
                }


                case OP_Fragment:
                {
		            Debug("OP_Fragment");
                    Debug("Raw Data");
                    //Debug(Utils.HexDump(Payload));

                    byte[] Uncompressed;

                    if (!SubPacket && (Payload[2] == 0x5a))
                    {
		                Debug("Compressed");
                        Uncompressed = DecompressPacket(Payload, 3);
                    }
                    else if (!SubPacket && (Payload[2] == 0xa5))
                    {
		                Debug("0xa5");
                        Uncompressed = new byte[Payload.Length - 5];

			            Array.Copy(Payload, 3, Uncompressed, 0, Payload.Length - 5);
                    }
                    else
                    {
                        Uncompressed = new byte[Payload.Length - 2];

			            Array.Copy(Payload, 2, Uncompressed, 0, Payload.Length - 2);
                    }
		            Debug("Uncompressed data.");
		            //Debug(Utils.HexDump(Uncompressed));

                    if (Direction == PacketDirection.ClientToServer)
                    {
                        int CSFragmentSeq = (Uncompressed[0] * 256) + Uncompressed[1];

                        if (CSFragmentSeq > GetExpectedSeq(Direction))
                            AddToCache(CSFragmentSeq, Direction, Payload, PacketTime, SubPacket);
                        else
                        {
                            AdvanceSeq(Direction);
                            Debug("Ignoring Fragmented packet from client ... can't handle it yet.");
                        }

                        break;
                    }

                    if (FragmentSeq == -1)
                    {
		                Debug("First fragment.");
                        FragmentSeq = (Uncompressed[0] * 256) + Uncompressed[1];
                        Debug("FragmentSeq is " + FragmentSeq + " Expecting " + GetExpectedSeq(Direction));
                        if (FragmentSeq != GetExpectedSeq(Direction))
                        {			                
                            if (FragmentSeq > GetExpectedSeq(Direction))
                            {
                                if((FragmentSeq - GetExpectedSeq(Direction)) < 1000)
                                    AddToCache(FragmentSeq, Direction, Payload, PacketTime, SubPacket);
                                else
                                {
			                        Log("Giving up on seeing expected fragment.");

                                    ErrorsInStream = true;

				                    FragmentSeq = -1;

                                    AdvanceSeq(Direction);
                                }
                            }
                            FragmentSeq = -1;
                                                        
                            break;
                        }
                        else
                            AdvanceSeq(Direction);

                        FragmentedPacketSize = Uncompressed[2] * 0x1000000 + Uncompressed[3] * 0x10000 + Uncompressed[4] * 0x100 + Uncompressed[5];

			            if((FragmentedPacketSize == 0) || (FragmentedPacketSize > 1000000))
			            {
			                Debug("Got a fragmented packet of size " + FragmentedPacketSize +". Discarding.");

			                ErrorsInStream = true;

			                FragmentSeq = -1;

			                break;
			            }
                        FragmentedBytesCollected = Uncompressed.Length - 6;
                        
			            Debug("Total packet size is " + FragmentedPacketSize);

			            if((Uncompressed.Length - 6) > FragmentedPacketSize)
			            {
			                Log("Mangled fragment.");

			                ErrorsInStream = true;

			                FragmentSeq = -1;

			                break;
			            }

                        Fragments = new byte[FragmentedPacketSize];

                        Debug("Copying " + (Uncompressed.Length - 6) + " bytes to Fragments starting at index 0");

			            Array.Copy(Uncompressed, 6, Fragments, 0, Uncompressed.Length - 6);
                    }
                    else
                    {
                        int LastSeq = FragmentSeq;

                        FragmentSeq = (Uncompressed[0] * 256) + Uncompressed[1];

                        Debug("FragmentSeq is " + FragmentSeq + ". Expecting " + GetExpectedSeq(Direction));

                        if (FragmentSeq != GetExpectedSeq(Direction))
                        {
                            if (FragmentSeq > GetExpectedSeq(Direction))
                            {
                                if((FragmentSeq - GetExpectedSeq(Direction)) < 1000)
                                    AddToCache(FragmentSeq, Direction, Payload, PacketTime, SubPacket);
				                else
				                {
    			                    Log("Giving up on seeing expected fragment.");

			                        ErrorsInStream = true;

                                    AdvanceSeq(Direction);

				                    FragmentSeq = -1;
				                }
                            }                            
                            break;
                        }
                        else
                            AdvanceSeq(Direction);

                        Debug("Copying " + (Uncompressed.Length - 2) + " bytes from Uncompressed to Fragments starting at " + FragmentedBytesCollected);

			            if((Uncompressed.Length - 2) > (Fragments.Length - FragmentedBytesCollected))
			            {
			                Log("Mangled fragment. Discarding.");

                            ErrorsInStream = true;

			                FragmentSeq = -1;

			                break;
			            }

                        Array.Copy(Uncompressed, 2, Fragments, FragmentedBytesCollected, Uncompressed.Length - 2);

                        FragmentedBytesCollected += Uncompressed.Length - 2;

                        if (FragmentedBytesCollected == FragmentedPacketSize)
                        {
                            Debug("Got whole packet.");

                            if ((Fragments[0] == 0x00) && (Fragments[1] == 0x019))
                            {
                                Debug("Multi packet.");

                                int BufferPosition = 2;

                                while (BufferPosition < Fragments.Length)
                                {
                                    int Size = 0;

                                    if (Fragments[BufferPosition] == 0xff)
                                    {
                                        if(Fragments[BufferPosition + 1] == 0x01)
                                            Size = 256 + Fragments[BufferPosition + 2];
                                        else
                                            Size = Fragments[BufferPosition + 2];

                                        BufferPosition += 3;
                                    }
                                    else
                                        Size = Fragments[BufferPosition++];

                                    int AppOpCode = Fragments[BufferPosition++] + (Fragments[BufferPosition++] * 256);
                                                                        
                                    ProcessAppPacket(srcIp, dstIp, srcPort, dstPort, AppOpCode, Size - 2, Fragments, BufferPosition, Direction, PacketTime);
                                    
                                    BufferPosition = BufferPosition + (Size - 2);
                                }
                            }
                            else
                            {
                                int AppOpCode = Fragments[0] + (Fragments[1] * 256);

                                byte[] NewPacket = new byte[Fragments.Length - 2];

				                Array.Copy(Fragments, 2, NewPacket, 0, Fragments.Length - 2);
                                
                                ProcessAppPacket(srcIp, dstIp, srcPort, dstPort, AppOpCode, NewPacket.Length, NewPacket, 0, Direction, PacketTime);
                            }
                            Debug("Reseting FragmentSeq to -1");
                            FragmentSeq = -1;
                        }
                    }
                
                    break;
                }
                case OP_OutOfOrderAck:
                {
                    int Seq = Payload[2] * 256 + Payload[3];
		            Debug("OP_OutOfOrder " + Seq);

                    break;
                }
                case OP_Ack:
                {
                    int Seq;

                    if(Payload.Length > 4)
                        Seq = Payload[3] * 256 + Payload[4];
                    else
                        Seq = Payload[2] * 256 + Payload[3];
                   
                    string DirectionString;

		            if(Direction == PacketDirection.ClientToServer)
		                DirectionString = "Client to Server";
                    else
		                DirectionString = "Server to Client";

		            Debug("OP_Ack, Seq " + Seq + " " + DirectionString);

		            //Debug(Utils.HexDump(Payload));
                    
                    break;
                }
                default:
                    if (OpCode > 0xff)
                    {
                        Debug("Unencapsulated EQ Application OpCode.");
                        // EQApplication OpCode
                        int AppOpCode = Payload[1] * 256 + Payload[0];

                        byte[] NewPacket = new byte[Payload.Length - 2];

			            Array.Copy(Payload, 2, NewPacket, 0, Payload.Length - 2);

                        ProcessAppPacket(srcIp, dstIp, srcPort, dstPort, AppOpCode, NewPacket.Length, NewPacket, 0, Direction, PacketTime);
                    }
                    else
                    {
                        Debug("OP_Unknown (" + OpCode.ToString("x") + ")");
                        //Debug(Utils.HexDump(Payload));
                    }
                    break;
            }

            if (!Cached && !CacheEmpty())
                ProcessCache();        
        }

        public void ProcessAppPacket(System.Net.IPAddress srcIp, System.Net.IPAddress dstIp, ushort srcPort, ushort dstPort, int InOpCode, int BufferSize, byte[] Source, int Offset, PacketDirection Direction, DateTime PacketTime)
        {
            EQApplicationPacket app = new EQApplicationPacket(srcIp, dstIp, srcPort, dstPort, InOpCode, BufferSize, Source, Offset, Direction, PacketTime);

            if (!Identified)
            {
                IdentificationStatus TempStatus = Identifier(InOpCode, BufferSize, Direction);

                if (!PermaLocked && (TempStatus > IdentificationStatus.No))
                {
                    Log("-- Permalocking to this stream.");
                    PermaLocked = true;
                }
                Identified = (TempStatus == IdentificationStatus.Yes);
            }                                               
            AddPacket(app);
        }

        public static byte[]  DecompressPacket(byte[] Payload, int Offset)
        {
            MemoryStream ms = new MemoryStream(Payload.GetUpperBound(0) - Offset);

            ms.Write(Payload, Offset, Payload.GetUpperBound(0) - Offset);

            ms.Seek(0, System.IO.SeekOrigin.Begin);

            ZInputStream zs = new ZInputStream(ms);

            int UncompressedSize;

	        byte[] Uncompressed = new byte[4096];

            try
            {
                UncompressedSize = zs.read(Uncompressed, 0, 4096);
            }
            catch
            {
		        Array.Copy(Payload, Offset - 1, Uncompressed, 0, Payload.Length - (Offset - 1));

                UncompressedSize = Payload.Length - (Offset - 1);
            }

            zs.Close();

            zs.Dispose();

            ms.Close();

            ms.Dispose();

            Array.Resize(ref Uncompressed, UncompressedSize);

            return Uncompressed;
        }        
    }
}
