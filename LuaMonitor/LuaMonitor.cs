using LuaInterface;
using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEditor;
using System.Linq;

namespace Pangu.Tools.LuaMonitor
{
    public class LuaMonitor : EditorWindow
    {
        private bool isInited = false;
        private MainView mainView = new MainView();

        [MenuItem("LuaMonitor/start")]
        public static void OpenWindow()
        {
            GetWindow(typeof(LuaMonitor), false, "LuaMonitor");
        }

        private void OnGUI()
        {
            try
            {
                if (!isInited && EditorApplication.isPlaying)
                {
                    Debugger.useLog = true;
                    Snapshot.snapshot_initialize(AppFacade.luaManager.GetLuaStateOnlyForLuaMonitor().L);
                    isInited = true;
                    mainView.Init(null);
                }
                if (!EditorApplication.isPlaying)
                {
                    isInited = false;
                }
                mainView.Draw(this.position);
            }
            catch (Exception e)
            {
                Debugger.LogError(e.Message + e.StackTrace);
            }
        }

        private void OnDestroy()
        {
            if (isInited)
            {
                Snapshot.snapshot_clear();
                mainView.OnDestroy();
            }
        }
    }
}