using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

namespace Pangu.Tools.LuaMonitor
{
    public class HierarchyView: AbstractSmallWindow
    {
        protected HashSet<SnapshotNode> isFoldSet = new HashSet<SnapshotNode>();

        private Vector2 scrollPosition = Vector2.zero;
        private float maxY = 0f;
        private const float SCROLL_WHEEL_SENSITIVE = 15f;
        private List<Drawing> drawings;
        private Rect oldPosition;

        protected override void OnSnapshotChange(Snapshot snapshot)
        {
            this.snapshot = snapshot;
            drawings = null;
        }

        public override void Draw(Rect area)
        {
            
            if (snapshot.root != null)
            {
                scrollPosition = EditorGUILayout.BeginScrollView(scrollPosition);
                Draw(snapshot.root, 0);
                EditorGUILayout.EndScrollView();
            }
            else
            {
                if (area.width != oldPosition.width && Event.current.type == EventType.Layout || drawings == null)
                {
                    drawings = null;
                    oldPosition = area;
                    scrollPosition = Vector2.zero;
                    maxY = 0f;
                }

                if (drawings == null)
                {
                    drawings = Draw(snapshot.list, area);
                    foreach (Drawing draw in drawings)
                    {
                        draw.rect.y += maxY;
                        maxY += draw.rect.height;
                    }
                }
                Rect r = area;
                r.x = area.x + area.width - 16;
                r.width = 16;
                if (maxY > area.height)
                {
                    scrollPosition.y = GUI.VerticalScrollbar(r, scrollPosition.y, area.height, 0, maxY);
                    if (Event.current.isScrollWheel)
                    {
                        scrollPosition.y += Event.current.delta.y * SCROLL_WHEEL_SENSITIVE;
                        if (scrollPosition.y < 0)
                        {
                            scrollPosition.y = 0;
                        }
                        else if (scrollPosition.y > maxY)
                        {
                            scrollPosition.y = maxY;
                        }
                        Event.current.Use();
                    }
                }
                else
                {
                    scrollPosition.y = 0;
                }

                EditorGUILayout.BeginHorizontal();
                EditorGUILayout.LabelField(" ", GUILayout.Width(10));
                EditorGUILayout.BeginVertical();
                foreach (Drawing draw in drawings)
                {
                    Rect newRect = draw.rect;
                    newRect.y -= scrollPosition.y;
                    if (newRect.y + newRect.height > area.y && newRect.y < area.height)
                    {
                        draw.Draw();
                    }
                }
                EditorGUILayout.EndVertical();
                EditorGUILayout.EndHorizontal();
            }
        }

        protected void Draw(SnapshotNode node, long parentId)
        {
            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField(" ", GUILayout.Width(10));
            EditorGUILayout.BeginVertical();
            string text = string.Format("{0}  size:{1}({2}B)  total size:{3}({4}B)", 
                node.GetName(parentId),  
                EditorUtility.FormatBytes(node.thisNodeSize),
                node.thisNodeSize,
                EditorUtility.FormatBytes(node.totalSize),
                node.totalSize);
            bool isFold = true;
            if (node.children.Count > 0)
            {
                if (isFoldSet.Contains(node))
                {
                    isFold = false;
                }
                isFold = !EditorGUILayout.Foldout(!isFold, text);
                if (!isFold)
                {
                    isFoldSet.Add(node);
                }
                else
                {
                    if (isFoldSet.Contains(node))
                    {
                        isFoldSet.Remove(node);
                    }
                }
            }
            else
            {
                EditorGUILayout.LabelField(text);
            }
            if (!isFold)
            {
                for (int i = 0; i < node.children.Count; i++)
                {
                    Draw(node.children[i], node.id);
                }
            }
            EditorGUILayout.EndVertical();
            EditorGUILayout.EndHorizontal();
        }

        protected List<Drawing> Draw(List<SnapshotNode> list, Rect area)
        {
            List<Drawing> drawing = new List<Drawing>();
            foreach (SnapshotNode node in list)
            {
                string text = string.Format("{0}  size:{1}({2}B)  total size:{3}({4}B)",
                    node.GetName(0),
                    EditorUtility.FormatBytes(node.thisNodeSize),
                    node.thisNodeSize,
                    EditorUtility.FormatBytes(node.totalSize),
                    node.totalSize);
                drawing.Add(Drawing.CreateDrawing(new Rect(area.x, area.y, area.width, EditorGUIUtility.singleLineHeight), () =>
                {
                    EditorGUILayout.BeginVertical();
                    EditorGUILayout.LabelField(text);
                    EditorGUILayout.EndVertical();
                }));
            }
            return drawing;
        }
    }

    public class HierarchyAndPageView : AbstractSmallWindow
    {
        public override Snapshot snapshot { get { return subWindow[0].snapshot; } }

        public override void Draw(Rect area)
        {
            GUILayout.BeginVertical();
            subWindow[0].Draw(new Rect(area.x, area.y, area.width, EditorGUIUtility.singleLineHeight));
            subWindow[1].Draw(new Rect(area.x, area.y + EditorGUIUtility.singleLineHeight, area.width, area.height - EditorGUIUtility.singleLineHeight));
            GUILayout.EndVertical();
        }

        protected override void OnInit()
        {
            subWindow.Add(new PageView());
            subWindow.Add(new HierarchyView());
        }
    }
  
    public class CompositeHierarchyView : AbstractSmallWindow
    {
        public override Snapshot snapshot { get { return subWindow[0].snapshot; } }
        public override Snapshot snapshot2 { get { return subWindow[1].snapshot; } }

        public override void Draw(Rect area)
        {
            float width = area.width / 2;
            GUILayout.BeginHorizontal(GUILayout.MaxWidth(area.width));
            subWindow[0].Draw(new Rect(area.x, area.y, width, area.height));
            subWindow[1].Draw(new Rect(area.x + width, area.y, width, area.height));
            GUILayout.EndHorizontal();
        }

        protected override void OnInit()
        {
            subWindow.Add(new HierarchyAndPageView());
            subWindow.Add(new HierarchyAndPageView());
        }
    }
}