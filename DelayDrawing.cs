using System;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

namespace Pangu.Tools
{
    public class Drawing
    {
        private const int DEFAULT_WINDOW_WIDTH = 1500;
        public Rect rect;
        public Action Draw;

        Drawing() { }

        public static Drawing CreateDrawing(Rect rect, Action Draw)
        {
            Drawing draw = new Drawing();
            draw.rect = rect;
            draw.Draw = Draw;
            return draw;
        }

        public static Drawing CreateDrawing(int infoAmount, Action Draw)
        {
            Drawing draw = new Drawing();
            //后面那个是unity添加的间隔
            draw.rect = new Rect(0, 0, DEFAULT_WINDOW_WIDTH, (EditorGUIUtility.singleLineHeight + 5) * infoAmount + 2 * (infoAmount - 1));
            draw.Draw = Draw;
            return draw;
        }
    }
}