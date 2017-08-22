using System;
using UnityEngine;
using System.Collections.Generic;
using UnityEditor;
using System.Linq;

namespace Pangu.Tools.LuaMonitor
{
    public class TreeMapView: AbstractSmallWindow
    {
        private ZoomArea zoomArea;
        private Dictionary<string, Group> groups = new Dictionary<string, Group>();
        private List<Item> items = new List<Item>();
        private List<Mesh> cachedMeshes = new List<Mesh>();
        private Item selectedItem;
        private Group selectedGroup;
        private Item mouseDownItem;
        private Vector2 mouseTreemapPosition { get { return zoomArea.ViewToDrawingTransformPoint(Event.current.mousePosition); } }

        protected override void OnInit()
        {
            zoomArea = new ZoomArea(true)
            {
                vRangeMin = -110f,
                vRangeMax = 110f,
                hRangeMin = -110f,
                hRangeMax = 110f,
                hBaseRangeMin = -110f,
                vBaseRangeMin = -110f,
                hBaseRangeMax = 110f,
                vBaseRangeMax = 110f,
                shownArea = new Rect(-110f, -110f, 220f, 220f)
            };
        }

        protected override void OnSnapshotChange(Snapshot snapshot)
        {
            this.snapshot = snapshot;
            RefreshCaches();
            RefreshMesh();
        }

        protected override void OnNodeChange(SnapshotNode node)
        {
            if (snapshot.index.ContainsKey(node.id))
            {
                selectedItem = items.First(i => i.node == node);
                selectedGroup = selectedItem.group;
                RefreshCachedRects(false);
            }
            else
            {
                Debugger.LogError("该快照不含该值，需要在之后的版本中改进");
            }
        }

        protected override void OnGroupChange(Group group)
        {
            selectedItem = null;
            selectedGroup = group;
            RefreshCachedRects(false);
        }

        public override void Draw(Rect area)
        {
            if (hostWindow == null)
            {
                return;
            }

            zoomArea.rect = area;
            zoomArea.BeginViewGUI();

            GUI.BeginGroup(area);
            Handles.matrix = zoomArea.drawingToViewMatrix;
            HandleMouseClick();
            RenderTreemap();
            GUI.EndGroup();

            zoomArea.EndViewGUI();
        }

        private void HandleMouseClick()
        {
            if ((Event.current.type == EventType.MouseDown || Event.current.type == EventType.MouseUp) && Event.current.button == 0)
            {
                if (zoomArea.drawRect.Contains(Event.current.mousePosition))
                {
                    Group group = groups.Values.FirstOrDefault(i => i.position.Contains(mouseTreemapPosition));
                    Item item = items.FirstOrDefault(i => i.position.Contains(mouseTreemapPosition));

                    if (item != null && selectedGroup == item.group)
                    {
                        switch (Event.current.type)
                        {
                            case EventType.MouseDown:
                                mouseDownItem = item;
                                break;

                            case EventType.MouseUp:
                                if (mouseDownItem == item)
                                {
                                    if (NodeChangeEvent != null)
                                    {
                                        NodeChangeEvent(item.node);
                                    }
                                    Event.current.Use();
                                }
                                break;
                        }
                    }
                    else if (group != null)
                    {
                        switch (Event.current.type)
                        {
                            case EventType.MouseUp:
                                if (GroupChangeEvent != null)
                                {
                                    GroupChangeEvent(group);
                                }
                                Event.current.Use();
                                break;
                        }
                    }
                }
            }
        }
        
        private void RefreshCaches()
        {
            items.Clear();
            groups.Clear();

            foreach (SnapshotNode node in snapshot.index.Values)
            {
                string groupName = GetGroupName(node);
                if (!groups.ContainsKey(groupName))
                {
                    Group newGroup = new Group();
                    newGroup.name = groupName;
                    newGroup.items = new List<Item>();
                    groups.Add(groupName, newGroup);
                }
                Item item = new Item(node, groups[groupName]);
                items.Add(item);
                groups[groupName].items.Add(item);
            }
            
            foreach (Group group in groups.Values)
            {
                group.items.Sort();
            }

            items.Sort();
            RefreshCachedRects(true);
        }

        private void RefreshCachedRects(bool fullRefresh)
        {
            Rect space = new Rect(-100f, -100f, 200f, 200f);

            if (fullRefresh)
            {
                List<Group> groupsList = groups.Values.ToList();
                groupsList.Sort();
                float[] groupTotalValues = new float[groupsList.Count];
                for (int i = 0; i < groupsList.Count; i++)
                {
                    groupTotalValues[i] = groupsList.ElementAt(i).totalMemorySize;
                }

                Rect[] groupRects = Utility.GetTreemapRects(groupTotalValues, space);
                for (int groupIndex = 0; groupIndex < groupRects.Length; groupIndex++)
                {
                    Group group = groupsList[groupIndex];
                    group.position = groupRects[groupIndex];
                }
            }

            if (selectedGroup != null)
            {
                Rect[] rects = Utility.GetTreemapRects(selectedGroup.memorySizes, selectedGroup.position);

                for (int i = 0; i < rects.Length; i++)
                {
                    selectedGroup.items[i].position = rects[i];
                }
            }

            RefreshMesh();
        }

        private void CleanupMeshes()
        {
            if (cachedMeshes == null)
            {
                cachedMeshes = new List<Mesh>();
            }
            else
            {
                for (int i = 0; i < cachedMeshes.Count; i++)
                {
                    UnityEngine.Object.DestroyImmediate(cachedMeshes[i]);
                }

                cachedMeshes.Clear();
            }
        }

        private void RefreshMesh()
        {
            CleanupMeshes();

            const int maxVerts = 32000;
            Vector3[] vertices = new Vector3[maxVerts];
            Color[] colors = new Color[maxVerts];
            int[] triangles = new int[maxVerts * 6 / 4];

            int meshItemIndex = 0;
            int totalItemIndex = 0;

            List<ITreemapRenderable> visible = new List<ITreemapRenderable>();
            foreach (Group group in groups.Values)
            {
                if (group != selectedGroup)
                {
                    visible.Add(group);
                }
                else
                {
                    foreach (Item item in group.items)
                    {
                        visible.Add(item);
                    }
                }
            }

            foreach (ITreemapRenderable item in visible)
            {
                int index = meshItemIndex * 4;
                vertices[index++] = new Vector3(item.GetPosition().xMin, item.GetPosition().yMin, 0f);
                vertices[index++] = new Vector3(item.GetPosition().xMax, item.GetPosition().yMin, 0f);
                vertices[index++] = new Vector3(item.GetPosition().xMax, item.GetPosition().yMax, 0f);
                vertices[index++] = new Vector3(item.GetPosition().xMin, item.GetPosition().yMax, 0f);

                index = meshItemIndex * 4;
                var color = item.GetColor();
                if (item == selectedItem)
                    color *= 1.5f;

                colors[index++] = color;
                colors[index++] = color * 0.75f;
                colors[index++] = color * 0.5f;
                colors[index++] = color * 0.75f;

                index = meshItemIndex * 6;
                triangles[index++] = meshItemIndex * 4 + 0;
                triangles[index++] = meshItemIndex * 4 + 1;
                triangles[index++] = meshItemIndex * 4 + 3;
                triangles[index++] = meshItemIndex * 4 + 1;
                triangles[index++] = meshItemIndex * 4 + 2;
                triangles[index++] = meshItemIndex * 4 + 3;

                meshItemIndex++;
                totalItemIndex++;

                if (meshItemIndex >= maxVerts / 4 || totalItemIndex == visible.Count)
                {
                    Mesh mesh = new Mesh();
                    mesh.hideFlags = HideFlags.DontSaveInEditor | HideFlags.HideInHierarchy | HideFlags.NotEditable;
                    mesh.vertices = vertices;
                    mesh.triangles = triangles;
                    mesh.colors = colors;
                    cachedMeshes.Add(mesh);

                    vertices = new Vector3[maxVerts];
                    colors = new Color[maxVerts];
                    triangles = new int[maxVerts * 6 / 4];
                    meshItemIndex = 0;
                }
            }
        }

        public void RenderTreemap()
        {
            if (cachedMeshes == null)
                return;

            Material mat = (Material)EditorGUIUtility.LoadRequired("SceneView/2DHandleLines.mat");
            mat.SetPass(0);

            for (int i = 0; i < cachedMeshes.Count; i++)
            {
                Graphics.DrawMeshNow(cachedMeshes[i], Handles.matrix);
            }
            RenderLabels();
        }

        private void RenderLabels()
        {
            if (groups == null)
                return;

            GUI.color = Color.black;
            Matrix4x4 mat = zoomArea.drawingToViewMatrix;

            foreach (Group group in groups.Values)
            {
                if (Utility.IsInside(group.position, zoomArea.shownArea))
                {
                    if (selectedItem != null && selectedItem.group == group)
                    {
                        RenderGroupItems(group);
                    }
                    else
                    {
                        RenderGroupLabel(group);
                    }
                }
            }

            GUI.color = Color.white;
        }

        private void RenderGroupLabel(Group group)
        {
            Matrix4x4 mat = zoomArea.drawingToViewMatrix;

            Vector3 p1 = mat.MultiplyPoint(new Vector3(group.position.xMin, group.position.yMin));
            Vector3 p2 = mat.MultiplyPoint(new Vector3(group.position.xMax, group.position.yMax));

            if (p2.x - p1.x > 30f)
            {
                Rect rect = new Rect(p1.x, p2.y, p2.x - p1.x, p1.y - p2.y);
                GUI.Label(rect, group.GetLabel());
            }
        }

        private void RenderGroupItems(Group group)
        {
            Matrix4x4 mat = zoomArea.drawingToViewMatrix;

            foreach (Item item in group.items)
            {
                if (Utility.IsInside(item.position, zoomArea.shownArea))
                {
                    Vector3 p1 = mat.MultiplyPoint(new Vector3(item.position.xMin, item.position.yMin));
                    Vector3 p2 = mat.MultiplyPoint(new Vector3(item.position.xMax, item.position.yMax));

                    if (p2.x - p1.x > 30f)
                    {
                        Rect rect = new Rect(p1.x, p2.y, p2.x - p1.x, p1.y - p2.y);
                        string row1 = item.group.name;
                        string row2 = EditorUtility.FormatBytes(item.memorySize);
                        GUI.Label(rect, row1 + "\n" + row2);
                    }
                }
            }
        }

        private string GetGroupName(SnapshotNode node)
        {
            string name = "";
            switch (node.type)
            {
                case SnapshotType.SNAPSHOT_C_FUNCTION:
                    if (node.thisNodeSize > 500)
                    {
                        name = "Big C Function (size > 500B)";
                    }
                    else
                    {
                        name = "Small C Function  (size <= 500B)";
                    }
                    break;
                case SnapshotType.SNAPSHOT_LUA_FUNCTION:
                    if (node.thisNodeSize > 500)
                    {
                        name = "Big Lua Function (size > 500B)";
                    }
                    else
                    {
                        name = "Small Lua Function (size <= 500B)";
                    }
                    break;
                case SnapshotType.SNAPSHOT_MASK:
                    name = "Mask";
                    break;
                case SnapshotType.SNAPSHOT_STRING:
                    if (node.thisNodeSize > 500)
                    {
                        name = "Big String (size > 500B)";
                    }
                    else
                    {
                        name = "Small String (size <= 500B)";
                    }
                    break;
                case SnapshotType.SNAPSHOT_TABLE:
                    if (node.thisNodeSize > 500)
                    {
                        name = "Big Table (size > 500B)";
                    }
                    else if (node.thisNodeSize <= 500 && node.thisNodeSize > 100)
                    {
                        name = "Middle Table (100B < size <= 500B)";
                    }
                    else
                    {
                        name = "Small Table (size <= 100B)";
                    }
                    break;
                case SnapshotType.SNAPSHOT_THREAD:
                    name = "Thread";
                    break;
                case SnapshotType.SNAPSHOT_USERDATA:
                    name = "User Data";
                    break;
            }
            return name;
        }
    }

    interface ITreemapRenderable
    {
        Color GetColor();
        Rect GetPosition();
        string GetLabel();
    }

    public class Group : IComparable<Group>, ITreemapRenderable
    {
        public string name;
        public Rect position;
        public List<Item> items;
        private float _totalMemorySize = -1;

        public float totalMemorySize
        {
            get
            {
                if (_totalMemorySize != -1)
                    return _totalMemorySize;

                long result = 0;
                foreach (Item item in items)
                {
                    result += item.memorySize;
                }
                _totalMemorySize = result;
                return result;
            }
        }

        public float[] memorySizes
        {
            get
            {
                float[] result = new float[items.Count];
                for (int i = 0; i < items.Count; i++)
                {
                    result[i] = items[i].memorySize;
                }
                return result;
            }
        }

        public Color color
        {
            get { return Utility.GetColorForName(name); }
        }

        public int CompareTo(Group other)
        {
            return (int)(other.totalMemorySize - totalMemorySize);
        }

        public Color GetColor()
        {
            return color;
        }

        public Rect GetPosition()
        {
            return position;
        }

        public string GetLabel()
        {
            string row1 = name;
            string row2 = EditorUtility.FormatBytes((long)totalMemorySize);
            return row1 + "\n" + row2;
        }
    }

    public class Item : IComparable<Item>, ITreemapRenderable
    {
        public Group group;
        public Rect position;
        public SnapshotNode node;

        public uint memorySize { get { return node.thisNodeSize; } }
        public string name { get { return node.GetName(0); } }

        public Item(SnapshotNode node, Group group)
        {
            this.node = node;
            this.group = group;
        }

        public int CompareTo(Item other)
        {
            return (int)(group != other.group ? other.group.totalMemorySize - group.totalMemorySize : other.memorySize - memorySize);
        }

        public Color GetColor()
        {
            return group.color;
        }

        public Rect GetPosition()
        {
            return position;
        }

        public string GetLabel()
        {
            string row1 = group.name;
            string row2 = EditorUtility.FormatBytes(memorySize);
            return row1 + "\n" + row2;
        }
    }

    public class Inspector: AbstractSmallWindow
    {
        private SnapshotNode selectedNode;
        private Vector2 scrollPosition = Vector2.zero;

        protected override void OnSnapshotChange(Snapshot snapshot)
        {
            this.snapshot = snapshot;
            this.selectedNode = null;
        }

        protected override void OnNodeChange(SnapshotNode node)
        {
            this.selectedNode = node;
        }
        
        public override void Draw(Rect area)
        {
            GUILayout.BeginArea(area);
            scrollPosition = GUILayout.BeginScrollView(scrollPosition);
            if (selectedNode == null)
            {
                GUILayout.Label("Select an object to see more info");
            }
            else
            {
                EditorGUILayout.LabelField(string.Format("Name: {0}", selectedNode.GetName(0)));
                EditorGUILayout.LabelField(string.Format("Other Name: "));
                selectedNode.GetDistinctName().ForEach(c => EditorGUILayout.LabelField("    "+c));
                EditorGUILayout.LabelField(string.Format("Type: {0}", selectedNode.type.ToString()));
                EditorGUILayout.LabelField(string.Format("Size: {0} ({1} Byte)", EditorUtility.FormatBytes(selectedNode.thisNodeSize), selectedNode.thisNodeSize.ToString()));
                EditorGUILayout.LabelField(string.Format("TotalSize: {0} ({1} Byte)", EditorUtility.FormatBytes(selectedNode.totalSize), selectedNode.totalSize.ToString()));
                EditorGUILayout.LabelField("Reference: ");
                foreach (SnapshotNode child in selectedNode.children)
                {
                    if (GUILayout.Button(child.GetName(selectedNode.id)))
                    {
                        if (NodeChangeEvent != null)
                        {
                            NodeChangeEvent(child);
                        }
                    }
                }
                EditorGUILayout.LabelField("Reference By: ");
                foreach (long parent in selectedNode.parents)
                {
                    if (!snapshot.index.ContainsKey(parent))
                    {
                        continue;
                    }
                    SnapshotNode node = snapshot.index[parent];
                    if (GUILayout.Button(node.GetName(selectedNode.id)))
                    {
                        if (NodeChangeEvent != null)
                        {
                            NodeChangeEvent(node);
                        }
                    }
                }
            }
            GUILayout.EndScrollView();
            GUILayout.EndArea();
        }
    }

    public class CompositeTreeMapView : AbstractSmallWindow
    {
        public override Snapshot snapshot { get { return subWindow[0].snapshot; } }
        protected override void OnInit()
        {
            subWindow.Add(new TreeMapView());
            subWindow.Add(new Inspector());
            subWindow.Add(new PageView());
        }

        public override void Draw(Rect area)
        {
            float width = area.width / 5 * 3;
            subWindow[0].Draw(new Rect(area.x, area.y + EditorGUIUtility.singleLineHeight, width, area.height * 0.9f));
            subWindow[1].Draw(new Rect(area.x + width, area.y + EditorGUIUtility.singleLineHeight * 2, area.width - width, area.height * 0.9f));
            subWindow[2].Draw(new Rect(area.x, area.y, area.width, EditorGUIUtility.singleLineHeight));
        }
    }
}
