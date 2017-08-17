using LuaInterface;
using System;
using System.Linq;
using System.Collections.Generic;
using System.Runtime.InteropServices;


namespace Pangu.Tools.LuaMonitor
{
    public enum SnapshotType
    {

        /// SNAPSHOT_TABLE -> 0
        SNAPSHOT_TABLE = 0,

        /// SNAPSHOT_LUA_FUNCTION -> 1
        SNAPSHOT_LUA_FUNCTION = 1,

        /// SNAPSHOT_C_FUNCTION -> 2
        SNAPSHOT_C_FUNCTION = 2,

        /// SNAPSHOT_USERDATA -> 3
        SNAPSHOT_USERDATA = 3,

        /// SNAPSHOT_THREAD -> 4
        SNAPSHOT_THREAD = 4,

        /// SNAPSHOT_STRING -> 5
        SNAPSHOT_STRING = 5,

        /// SNAPSHOT_MASK -> 7
        SNAPSHOT_MASK = 7,
    }

    public class SnapshotNode : IComparable<SnapshotNode>
    {
        public long id;
        public SnapshotType type;
        public uint thisNodeSize = 0;
        private uint _totalSize = 0;
        private string name = "";
        public Dictionary<long, string> nameDict = new Dictionary<long, string>();
        public List<SnapshotNode> children = new List<SnapshotNode>();

        public bool isVisitedInRemoveCircle = false;
        private long CalculatedInNodeId = 0;

        public long[] parents;
        public string[] reference;

        public uint totalSize
        {
            get
            {
                if (_totalSize == 0)
                {
                    _totalSize = GetSizeInHierarchy(id);
                }
                return _totalSize;
            }
        }

        public string GetName(long parentId)
        {
            if (name == "" && nameDict.Values.Count != 0)
            {
                //if (!nameDict.ContainsKey(parentId) || Regex.IsMatch(nameDict[parentId], @"\[.*?\]"))
                if (!nameDict.ContainsKey(parentId))
                {
                    Dictionary<string, int> times = new Dictionary<string, int>();
                    foreach (string name in nameDict.Values)
                    {
                        if (times.ContainsKey(name))
                        {
                            times[name]++;
                        }
                        else
                        {
                            times[name] = 1;
                        }
                    };
                    KeyValuePair<string, int> max = times.ElementAt(0);
                    foreach (KeyValuePair<string, int> p in times)
                    {
                        if (p.Value > max.Value)
                        {
                            max = p;
                        }
                    }
                    name = max.Key;
                }
                else
                {
                    name = nameDict[parentId];
                }
            }
            return name;
        }

        public List<string> GetDistinctName()
        {
            HashSet<string> nameHashSet = new HashSet<string>();
            foreach (string name in nameDict.Values)
            {
                if (!nameHashSet.Contains(name))
                {
                    nameHashSet.Add(name);
                }
            }
            return nameHashSet.ToList();
        }

        protected uint GetSizeInHierarchy(long root)
        {
            if (CalculatedInNodeId == root)
            {
                return 0;
            }
            CalculatedInNodeId = root;
            uint tempSize = 0;
            for (int i = 0; i < children.Count; i++)
            {
                tempSize += children[i].GetSizeInHierarchy(root);
            }
            return tempSize + thisNodeSize;
        }

        public SnapshotNode(long id, SnapshotType type, uint thisNodeSize, long[] parents, string[] reference)
        {
            this.id = id;
            this.type = type;
            this.thisNodeSize = thisNodeSize;
            this.parents = parents;
            this.reference = reference;
        }

        public int CompareTo(SnapshotNode other)
        {
            return (int)(other.totalSize - totalSize);
            //use this when you don't remove circle
            //return (int)(other.thisNodeSize - thisNodeSize);
        }
    }

    public class Snapshot
    {
        #region luasnapshot
#if (UNITY_IOS || UNITY_TVOS) && !UNITY_EDITOR
        /*
        [DllImport("__Internal")]
        private static extern void snapshot_initialize(IntPtr L);

        [DllImport("__Internal")]
        private static extern int snapshot_capture(IntPtr L);

        [DllImport("__Internal")]
        private static extern void snapshot_clear();

        [DllImport("__Internal")]
        private static extern int snapshot_get_gcobj_num();

        [DllImport("__Internal")]
        private static extern int snapshot_get_gcobjs(int len, IntPtr arrayOut);

        [DllImport("__Internal")]
        private static extern int snapshot_get_parent_num(int index);

        [DllImport("__Internal")]
        private static extern int snapshot_get_parents(int index, int len, IntPtr arrayPOut, IntPtr arrayROut);
        */
#else
        [DllImport("snapshot")]
        public static extern void snapshot_initialize(IntPtr L);

        [DllImport("snapshot")]
        public static extern int snapshot_capture(IntPtr L);

        [DllImport("snapshot")]
        public static extern void snapshot_clear();

        [DllImport("snapshot")]
        private static extern int snapshot_get_gcobj_num();

        [DllImport("snapshot")]
        private static extern int snapshot_get_gcobjs(int len, IntPtr arrayOut);
#endif


        [StructLayoutAttribute(LayoutKind.Sequential)]
        public struct SnapshotNodeWrapper
        {

            /// void*
            public IntPtr address;

            /// SnapshotType->_SnapshotType
            public SnapshotType type;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string Debuggerinfo;

            /// unsigned int
            public uint size;

            /// void**
            public IntPtr parents;

            /// char**
            public IntPtr reference;

            /// unsigned int
            public uint parentNum;
        }
        #endregion
        public string name = "";
        public Dictionary<long, SnapshotNode> index = new Dictionary<long, SnapshotNode>();
        public List<SnapshotNode> list = new List<SnapshotNode>();
        public SnapshotNode root;

        public static Snapshot GetSnapshot(LuaState lua)
        {
            if (snapshot_capture(lua.L) == -1)
            {
                return null;
            }
            int number = snapshot_get_gcobj_num();
            if (number == -1)
            {
                return null;
            }
            Snapshot snapshot = new Snapshot();

            //将c结构转换为c#结构
            int wrapperSize = Marshal.SizeOf(typeof(SnapshotNodeWrapper));
            IntPtr wrapperPtr = Marshal.AllocHGlobal(wrapperSize * number);
            snapshot_get_gcobjs(number, wrapperPtr);
            for (int i = 0; i < number; i++)
            {
                SnapshotNodeWrapper wrapper = (SnapshotNodeWrapper)Marshal.PtrToStructure((IntPtr)((UInt32)wrapperPtr + i * wrapperSize), typeof(SnapshotNodeWrapper));
                SnapshotNode node = new SnapshotNode(wrapper.address.ToInt64(), wrapper.type, wrapper.size, new long[wrapper.parentNum], new string[wrapper.parentNum]);
                for (int j = 0; j < wrapper.parentNum; j++)
                {
                    node.parents[j] = ((IntPtr)Marshal.PtrToStructure((IntPtr)((UInt32)wrapper.parents + j * Marshal.SizeOf(typeof(IntPtr))), typeof(IntPtr))).ToInt64();
                    node.reference[j] = Marshal.PtrToStringAnsi((IntPtr)Marshal.PtrToStructure((IntPtr)((UInt32)wrapper.reference + j * Marshal.SizeOf(typeof(IntPtr))), typeof(IntPtr)));
                }
                snapshot.index[node.id] = node;
            }

            foreach (KeyValuePair<long, SnapshotNode> kyPair in snapshot.index)
            {
                SnapshotNode thisNode = kyPair.Value;
                for (int i = 0; i < thisNode.parents.Length; i++)
                {
                    if (thisNode.parents[i] == 0)
                    {
                        snapshot.root = thisNode;
                    }
                    if (!snapshot.index.ContainsKey(thisNode.parents[i]))
                    {
                        thisNode.nameDict[thisNode.parents[i]] = thisNode.reference[i];
                        continue;
                    }
                    snapshot.index[thisNode.parents[i]].children.Add(thisNode);
                    thisNode.nameDict[thisNode.parents[i]] = thisNode.reference[i];
                }
            }

            RemoveCircle(snapshot.root);

            foreach (SnapshotNode node in snapshot.index.Values)
            {
                node.children.Sort();
            }

            snapshot.name = DateTime.Now.ToShortTimeString();
            
            snapshot_clear();
            return snapshot;
        }

        public static Snapshot GetIntersection(Snapshot first, Snapshot second)
        {
            if (first.index.Count > second.index.Count)
            {
                Snapshot temp = first;
                first = second;
                second = temp;
            }
            Snapshot snapshot = new Snapshot();
            foreach (SnapshotNode node in first.index.Values)
            {
                if (second.index.ContainsKey(node.id))
                {
                    snapshot.index[node.id] = node;
                }
            }
            if (snapshot.index.Count == 0)
            {
                SnapshotNode temp = new SnapshotNode(0, 0, 0, new long[] { 0 }, new string[] { "Empty" });
                snapshot.index[temp.id] = temp;
            }
            snapshot.list = snapshot.index.Values.ToList();
            snapshot.list.Sort();
            return snapshot;
        }

        public static Snapshot GetDifferenceSet(Snapshot first, Snapshot second)
        {
            Snapshot snapshot = new Snapshot();
            foreach (SnapshotNode node in first.index.Values)
            {
                if (!second.index.ContainsKey(node.id))
                {
                    snapshot.index[node.id] = node;
                }
                else if (Setting.IS_DIFF_SET_CHECK_MEMORY && node.thisNodeSize != second.index[node.id].thisNodeSize)
                {
                    snapshot.index[node.id] = node;
                }
            }
            if (snapshot.index.Count == 0)
            {
                SnapshotNode temp = new SnapshotNode(0, 0, 0, new long[] { 99999999 }, new string[] { "Empty" });
                snapshot.index[temp.id] = temp;
            }
            snapshot.list = snapshot.index.Values.ToList();
            snapshot.list.Sort();
            return snapshot;
        }

        private static void RemoveCircle(SnapshotNode root)
        {
            Queue<SnapshotNode> BFSQueue = new Queue<SnapshotNode>();
            int tempNum = 1;
            root.isVisitedInRemoveCircle = true;
            BFSQueue.Enqueue(root);
            while (BFSQueue.Count != 0)
            {
                SnapshotNode node = BFSQueue.Dequeue();
                tempNum--;
                node.children.RemoveAll(c => c.isVisitedInRemoveCircle);
                node.children.ForEach(c => BFSQueue.Enqueue(c));
                if (tempNum == 0)
                {
                    foreach (SnapshotNode queueNode in BFSQueue)
                    {
                        queueNode.isVisitedInRemoveCircle = true;
                    }
                    tempNum = BFSQueue.Count;
                }
            }
        }

    }
}