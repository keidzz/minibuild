// this the original code made in c# by me, thanks to this tutorial 
// https://www.youtube.com/watch?v=ZiHH_BvjoGk
// keep this file as reference

using Godot;
using Godot.Collections;
using System;
using System.Collections.Generic;
using System.Security.Cryptography.X509Certificates;


public struct JunctionInfo
{
    public Path3D path;
    public int point;

    public JunctionInfo(Path3D path, int point)
    {
        this.path = path;
        this.point = point;
    }

}

public struct JunctionEdge
{
    public Vector3 left;
    public Vector3 right;

    public Vector3 Center => (left + right) / 2;

    public JunctionEdge (Vector3 p1, Vector3 p2)
    {
        this.left = p1;
        this.right = p2;
    }
}

public class Intersection
{
    [Export]
    public List<JunctionInfo> junctions = new List<JunctionInfo>();

    [Export]
    public List<float> curves;

    public void AddJunction(RoadNode path, int point)
    {
        junctions.Add(new JunctionInfo(path, point));
    }

    public void Clear()
    {
        if (junctions.Count > 0)
        {
            junctions.Clear();
        }
    }

    internal IEnumerable<JunctionInfo> GetJunctions()
    {
        return junctions;
    }
}

[Tool]
public partial class IntersectionNode : Node3D
{
    private Transform3D _lastTransform;

    public Intersection intersection = new Intersection();

    [Export]
    public Array<RoadNode> paths = [];

    [Export]
    public Array<int> pointPath = [];

    [Export]
    public Array<float> curves = [];

    [Export]
    public StandardMaterial3D IntersectionMaterial;

    [Export]
    public bool showDebug;

    public List<Vector3> points = [];

    List<MeshInstance3D> debugInstances = [];

    public Vector3 center;

    public MeshInstance3D IntersectionMesh;
    public ArrayMesh IntersectionArrayMesh;

    public List<Vector3> curvePoints = [];

    public override void _Ready()
    {
        _lastTransform = Transform;
        IntersectionArrayMesh = new ArrayMesh();
        IntersectionMesh = new MeshInstance3D();
        IntersectionMesh.MaterialOverride = IntersectionMaterial;
        IntersectionMesh.Mesh = IntersectionArrayMesh;
        AddChild(IntersectionMesh);
        OnBuildJunction();
        getVerts();
        BuildMesh();
    }

    public override void _Process(double delta)
    {
        if (!Transform.IsEqualApprox(_lastTransform))
        {
            OnBuildJunction();
            getVerts();
            BuildMesh();
            if (showDebug) debugBoxes();
            if (!showDebug && debugInstances.Count > 0)
            {
                foreach (MeshInstance3D debugMesh in debugInstances)
                {
                    if (IsInstanceValid(debugMesh))
                    {
                        debugMesh.QueueFree();
                    }
                }
                debugInstances.Clear();
            }
            _lastTransform = Transform;
        }
    }
    public Vector3 EvaluatePosition(Vector3 p0, Vector3 p1, Vector3 p2, float t)
    {
        Vector3 q0 = p0.Lerp(p1, t);
        Vector3 q1 = p1.Lerp(p2, t);
        return q0.Lerp(q1, t);
    }


    public void getVerts()
    {
        curvePoints.Clear();

        List<JunctionEdge> junctionEdges = new List<JunctionEdge>();

        center = new Vector3();
        foreach (JunctionInfo junction in intersection.GetJunctions())
        {
            float t = junction.point == 0 ? 0f : 1f;
            RoadTools.SampleSplineWidth(junction.path, 8.0f, t, out Vector3 p1, out Vector3 p2);

            Vector3 globalP1 = ToLocal(junction.path.ToGlobal(p1));
            Vector3 globalP2 = ToLocal(junction.path.ToGlobal(p2));

            if (junction.point == 0)
            {
                junctionEdges.Add(new JunctionEdge(globalP1, globalP2));
            }
            else
            {
                junctionEdges.Add(new JunctionEdge(globalP2, globalP1));
            }

            center += globalP1;
            center += globalP2;
        }

        center = center / (junctionEdges.Count * 2);

        // Sort the junctions based o ntheir direction from the center
        junctionEdges.Sort((x, y) => SortPoints(center, x.Center, y.Center));

        Vector3 mid;
        Vector3 c;
        Vector3 b;
        Vector3 a;
        for (int j = 0; j < junctionEdges.Count; j++)
        {
            Vector3 currentLeft = junctionEdges[j].left;
            Vector3 currentRight = junctionEdges[j].right;

            int nextIndex = (j + 1) % junctionEdges.Count;
            Vector3 nextLeft = junctionEdges[nextIndex].left;
            Vector3 nextRight = junctionEdges[nextIndex].right;

            curvePoints.Add(currentLeft);
            curvePoints.Add(currentRight);

            Vector3 p0 = currentRight;
            Vector3 p2 = nextLeft;
            Vector3 p1 = p0.Lerp(p2, 0.5f).Lerp(center, curves[j]);


            int segments = 6;
            for (int i = 1; i < segments; i++)
            {
                float t = (float)i / segments;
                Vector3 pos = EvaluatePosition(p0, p1, p2, t); 
                curvePoints.Add(pos);
            }
        }
    }

    private int SortPoints(Vector3 center, Vector3 point1, Vector3 point2)
    {
        Vector3 direction1 = point1 - center;
        Vector3 direction2 = point2 - center;

        float angle1 = Mathf.Atan2(direction1.Z, direction1.X);
        float angle2 = Mathf.Atan2(direction2.Z, direction2.X);

        if (angle1 < 0) angle1 += 2 * Mathf.Pi;
        if (angle2 < 0) angle2 += 2 * Mathf.Pi;

        return angle1.CompareTo(angle2);
    }


    private void OnBuildJunction()
    {
        intersection.Clear();
        int length = paths.Count;

        for (int i = 0; i < length; i++)
        {
            intersection.AddJunction(paths[i], pointPath[i]);
        }
    }

    public void BuildMesh()
    {
        IntersectionArrayMesh.ClearSurfaces();
        Godot.Collections.Array surfaceArray = [];
        surfaceArray.Resize((int)Mesh.ArrayType.Max);
        List<Vector3> verts = [];
        List<Vector3> normals = [];
        List<int> indices = [];

        int pointsOffset = verts.Count;
        for (int j = 1; j <= curvePoints.Count; j++)
        {
            verts.Add(center);
            verts.Add(curvePoints[j - 1]);

            if (j == curvePoints.Count)
            {
                verts.Add(curvePoints[0]);
            }
            else
            {
                verts.Add(curvePoints[j]);
            }

            indices.Add(pointsOffset + ((j - 1) * 3) + 0);
            indices.Add(pointsOffset + ((j - 1) * 3) + 1);
            indices.Add(pointsOffset + ((j - 1) * 3) + 2);

            Vector3 normal = Vector3.Up;
            normals.AddRange(new List<Vector3> { normal, normal, normal });
        }

        surfaceArray[(int)Mesh.ArrayType.Vertex] = verts.ToArray();
        surfaceArray[(int)Mesh.ArrayType.Index] = indices.ToArray();
        surfaceArray[(int)Mesh.ArrayType.Normal] = normals.ToArray();


        IntersectionArrayMesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, surfaceArray);
    }

    public void debugBoxes()
    {
        if (debugInstances.Count > 0)
        {
            foreach (MeshInstance3D debugMesh in debugInstances)
            {
                if (IsInstanceValid(debugMesh))
                {
                    debugMesh.QueueFree();
                }
            }
            debugInstances.Clear();
        }

        foreach (Vector3 point in curvePoints)
        {
            MeshInstance3D mi = new MeshInstance3D();
            BoxMesh boxMesh = new BoxMesh();
            mi.Mesh = boxMesh;
            boxMesh.Size = new Vector3(0.1f, 0.1f, 0.1f);
            mi.Position = point;
            debugInstances.Add(mi);
            AddChild(mi);
        }

    }
}
