using LuaInterface;
using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEditor;
using System.Linq;

namespace Pangu.Tools.LuaMonitor
{
    public static class Const
    {
        public const string CHECK_MEMORY_PATH = "LuaScripts/pg/libs/check_memory.lua";
    }
    public abstract class AbstractSmallWindow
    {
        protected AbstractSmallWindow hostWindow;
        protected List<AbstractSmallWindow> subWindow = new List<AbstractSmallWindow>();

        public Action<Snapshot> SnapshotChangeEvent;
        public Action<SnapshotNode> NodeChangeEvent;
        public Action<Group> GroupChangeEvent;
        public virtual Snapshot snapshot { get; set; }
        public virtual Snapshot snapshot2 { get; set; }
        public virtual List<Snapshot> snapshotList { get { return hostWindow.snapshotList; } }


        public void Init(AbstractSmallWindow hostWindow)
        {
            if (hostWindow != null)
            {
                this.hostWindow = hostWindow;
            }
            OnInit();
            foreach (AbstractSmallWindow sub in subWindow)
            {
                sub.Init(this);
            }
            foreach (AbstractSmallWindow sub in subWindow)
            {
                sub.SnapshotChangeEvent += SnapshotChangeEvent;
                sub.NodeChangeEvent += NodeChangeEvent;
                sub.GroupChangeEvent += GroupChangeEvent;
            }
            SnapshotChangeEvent += OnSnapshotChange;
            NodeChangeEvent += OnNodeChange;
            GroupChangeEvent += OnGroupChange;

            if (hostWindow != null)
            {
                hostWindow.SnapshotChangeEvent += SnapshotChangeEvent;
                hostWindow.NodeChangeEvent += NodeChangeEvent;
                hostWindow.GroupChangeEvent += GroupChangeEvent;
            }
        }
        public void OnDestroy()
        {
            if (hostWindow != null)
            {
                hostWindow.SnapshotChangeEvent -= SnapshotChangeEvent;
                hostWindow.NodeChangeEvent -= NodeChangeEvent;
                hostWindow.GroupChangeEvent -= GroupChangeEvent;
            }
            SnapshotChangeEvent -= OnSnapshotChange;
            NodeChangeEvent -= OnNodeChange;
            GroupChangeEvent -= OnGroupChange;
            foreach (AbstractSmallWindow sub in subWindow)
            {
                sub.SnapshotChangeEvent -= SnapshotChangeEvent;
                sub.NodeChangeEvent -= NodeChangeEvent;
                sub.GroupChangeEvent -= GroupChangeEvent;
            }
            foreach (AbstractSmallWindow sub in subWindow)
            {
                sub.OnDestroy();
            }
        }
        public abstract void Draw(Rect area);
        protected virtual void OnInit() { }
        protected virtual void OnSnapshotChange(Snapshot snapshot) { }
        protected virtual void OnNodeChange(SnapshotNode node) { }
        protected virtual void OnGroupChange(Group group) { }
    }

    public class PageView : AbstractSmallWindow
    {
        public override void Draw(Rect area)
        {
            var c = GUI.backgroundColor;
            GUILayout.BeginHorizontal();
            for (int i = 0; i < snapshotList.Count; i++)
            {
                if (snapshot == snapshotList[i])
                {
                    GUI.backgroundColor = Color.cyan;
                }
                if (GUILayout.Button(snapshotList[i].name, GUILayout.MaxWidth(area.width)))
                {
                    if (SnapshotChangeEvent != null)
                    {
                        SnapshotChangeEvent(snapshotList[i]);
                    }
                }
                if (snapshot == snapshotList[i])
                {
                    GUI.backgroundColor = c;
                }
            }
            GUILayout.EndHorizontal();
        }

        protected override void OnSnapshotChange(Snapshot snapshot)
        {
            this.snapshot = snapshot;
        }
    }

    public class MainView: AbstractSmallWindow
    {
        private AbstractSmallWindow mainWindow;
        private AbstractSmallWindow hierarchyView = new CompositeHierarchyView();
        private AbstractSmallWindow treeMapView = new CompositeTreeMapView();
        private List<Snapshot> _snapshotList = new List<Snapshot>();
        public override List<Snapshot> snapshotList { get { return _snapshotList; } }

        bool temp = false;
        public override void Draw(Rect position)
        {
            GUILayout.BeginHorizontal();
            if (GUILayout.Button("检测内存上升"))
            {
                //Debugger.LogError("before: " + AppFacade.luaManager.LuaGC(LuaGCOptions.LUA_GCCOUNT) * 1024);
                //AppFacade.luaManager.LuaGC(LuaGCOptions.LUA_GCCOLLECT);
                //Debugger.LogError("after: " + AppFacade.luaManager.LuaGC(LuaGCOptions.LUA_GCCOUNT) * 1024);
                if (!temp)
                {
                    LuaState lua = AppFacade.luaManager.GetLuaStateOnlyForLuaMonitor();
                    lua.DoFile(Const.CHECK_MEMORY_PATH);
                    LuaFunction main = lua.GetFunction("SC_StartRecordAlloc");
                    main.Call(false);
                    temp = true;
                }
                else
                {
                    LuaState lua = AppFacade.luaManager.GetLuaStateOnlyForLuaMonitor();
                    LuaFunction main = lua.GetFunction("SC_StopRecordAllocAndDumpStat");
                    main.Call();
                    EditorUtility.DisplayDialog("", "输出文件memAlloc.csv至工作目录下,默认为Client目录下", "OK");
                    temp = false;
                }
            }
            if (GUILayout.Button("内存快照"))
            {
                if (!EditorApplication.isPlaying)
                {
                    return;
                }
                if (AppFacade.luaManager.GetLuaStateOnlyForLuaMonitor() == null)
                {
                    Debugger.LogError("lua state 为空");
                    return;
                }
                //Debugger.LogError("before: " + AppFacade.luaManager.LuaGC(LuaGCOptions.LUA_GCCOUNT) * 1024);
                //AppFacade.luaManager.LuaGC(LuaGCOptions.LUA_GCCOLLECT);
                //Debugger.LogError("after: " + AppFacade.luaManager.LuaGC(LuaGCOptions.LUA_GCCOUNT) * 1024);

                Snapshot snapshot = Snapshot.GetSnapshot(AppFacade.luaManager.GetLuaStateOnlyForLuaMonitor());
                if (snapshot == null)
                {
                    Debugger.LogError("snapshot 为空");
                    return;
                }
                snapshotList.Add(snapshot);
                snapshot.name = "S" + snapshotList.Count;
                if (SnapshotChangeEvent != null)
                {
                    SnapshotChangeEvent(snapshot);
                }
                Snapshot.snapshot_clear();
            }
            if (GUILayout.Button("交集"))
            {
                Snapshot snapshot1 = hierarchyView.snapshot;
                Snapshot snapshot2 = hierarchyView.snapshot2;
                if (snapshot1 == null || snapshot2 == null)
                {
                    return;
                }
                Snapshot snapshot = Snapshot.GetIntersection(snapshot1, snapshot2);
                snapshotList.Add(snapshot);
                snapshot.name = "(" + snapshot1.name + "∩" + snapshot2.name + ")";
                if (SnapshotChangeEvent != null)
                {
                    SnapshotChangeEvent(snapshot);
                }
            }
            if (GUILayout.Button("差集"))
            {
                Snapshot snapshot1 = hierarchyView.snapshot;
                Snapshot snapshot2 = hierarchyView.snapshot2;
                if (snapshot1 == null || snapshot2 == null)
                {
                    return;
                }
                Snapshot snapshot = Snapshot.GetDifferenceSet(snapshot1, snapshot2);
                snapshotList.Add(snapshot);
                snapshot.name = "(" + snapshot1.name + "-" + snapshot2.name + ")";
                if (SnapshotChangeEvent != null)
                {
                    SnapshotChangeEvent(snapshot);
                }
            }
            GUILayout.EndHorizontal();
            GUILayout.BeginHorizontal();
            if (GUILayout.Button("层级视图"))
            {
                mainWindow = hierarchyView;
            }
            if (GUILayout.Button("树图"))
            {
                mainWindow = treeMapView;
            }
            GUILayout.EndHorizontal();
            if (snapshotList.Count != 0)
            {
                mainWindow.Draw(new Rect(0f, EditorGUIUtility.singleLineHeight * 3 + 1, position.width, position.height - EditorGUIUtility.singleLineHeight * 3 - 1));
            }
        }

        protected override void OnInit()
        {
            mainWindow = hierarchyView;
            hierarchyView.Init(this);
            treeMapView.Init(this);

            hierarchyView.SnapshotChangeEvent += SnapshotChangeEvent;
            hierarchyView.NodeChangeEvent += NodeChangeEvent;
            hierarchyView.GroupChangeEvent += GroupChangeEvent;
            treeMapView.SnapshotChangeEvent += SnapshotChangeEvent;
            treeMapView.NodeChangeEvent += NodeChangeEvent;
            treeMapView.GroupChangeEvent += GroupChangeEvent;
        }
    }
}