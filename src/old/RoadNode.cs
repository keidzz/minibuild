// this the original code made in c# by me, thanks to this tutorial 
// https://www.youtube.com/watch?v=ZiHH_BvjoGk
// keep this file as reference


using Godot;
using Godot.Collections;
using System.Collections.Generic;


public class RoadTools
{
    public static bool Evaluate(Path3D path, float t, out Vector3 pathPosition, out Vector3 pathTangent, out Vector3 upVector)
    {
        pathPosition = Vector3.Zero;
        pathTangent = Vector3.Zero;
        upVector = Vector3.Zero;

        if (path.Curve == null || path.Curve.PointCount < 2)
            return false;

        t = Mathf.Clamp(t, 0.0f, 1.0f);

        float curveLength = path.Curve.GetBakedLength();
        float offset = t * curveLength;

        pathPosition = path.Curve.SampleBaked(offset);

        float deltaOffset = Mathf.Min(0.01f, curveLength * 0.01f);
        Vector3 nextPos;

        if (offset + deltaOffset <= curveLength)
        {
            nextPos = path.Curve.SampleBaked(offset + deltaOffset);
            pathTangent = (nextPos - pathPosition).Normalized();
        }
        else if (offset - deltaOffset >= 0)
        {
            Vector3 prevPos = path.Curve.SampleBaked(offset - deltaOffset);
            pathTangent = (pathPosition - prevPos).Normalized();
        }
        else
        {
            pathTangent = Vector3.Forward;
        }

        upVector = Vector3.Up;

        return true;
    }


    public static void getVerts(Path3D path, int resolution, float Width, out List<Vector3> m_vertsP1, out List<Vector3> m_vertsP2)
    {
        m_vertsP1 = new List<Vector3>();
        m_vertsP2 = new List<Vector3>();


        for (int i = 0; i <= resolution; i++)
        {
            float t = (float)i / resolution;
            SampleSplineWidth(path, Width, t, out Vector3 p1, out Vector3 p2);
            m_vertsP1.Add(p1);
            m_vertsP2.Add(p2);
        }
    }
    public static void SampleSplineWidth(Path3D path, float Width, float t, out Vector3 p1, out Vector3 p2)
    {
        Evaluate(path, t, out Vector3 position, out Vector3 tangent, out Vector3 upVector);
        Vector3 right = tangent.Cross(upVector).Normalized();

        p1 = position - (right * Width * 0.5f);
        p2 = position + (right * Width * 0.5f);

    }
}


[Tool]
public partial class RoadNode : Path3D
{
    [Export]
    public bool showDebug = false;

    public List<Vector3> m_vertsP1 = [];
    public List<Vector3> m_vertsP2 = [];

    [Export]
    public int resolution = 80;

    public MeshInstance3D RoadMesh;
    public ArrayMesh RoadArrayMesh;

    [Export]
    public float Width = 1.0f;

    [Export]
    public StandardMaterial3D RoadMaterial;

    List<MeshInstance3D> debugInstances = [];

    public override void _Ready()
    {

        RoadArrayMesh = new ArrayMesh();
        RoadMesh = new MeshInstance3D();
        RoadMesh.MaterialOverride = RoadMaterial;
        RoadMesh.Mesh = RoadArrayMesh;
        AddChild(RoadMesh);
        RoadTools.getVerts(this, resolution, Width, out m_vertsP1, out m_vertsP2);
        BuildMesh();
        CurveChanged += OnCurveChanged;
        
    }

    private void OnCurveChanged()
    {
        RoadTools.getVerts(this, resolution, Width, out m_vertsP1, out m_vertsP2);
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
    }


    public void BuildMesh()
    {
        RoadArrayMesh.ClearSurfaces();
        Godot.Collections.Array surfaceArray = [];
        surfaceArray.Resize((int)Mesh.ArrayType.Max);
        List<Vector3> verts = [];
        List<Vector3> normals = [];
        List<int> indices = [];
        int offset = 0;

        int length = m_vertsP2.Count;

        for (int i = 0; i < length - 1; i++)
        {
            Vector3 p1 = m_vertsP1[i];
            Vector3 p2 = m_vertsP2[i];
            Vector3 p3 = m_vertsP1[i + 1];
            Vector3 p4 = m_vertsP2[i + 1];

            int vertOffset = i * 4;

            int t1 = vertOffset + 0;
            int t2 = vertOffset + 2;
            int t3 = vertOffset + 3;

            int t4 = vertOffset + 3;
            int t5 = vertOffset + 1;
            int t6 = vertOffset + 0;

            verts.AddRange(new List<Vector3> { p1, p2, p3, p4 });

            Vector3 normal = Vector3.Up;
            normals.AddRange(new List<Vector3> { normal, normal, normal, normal });

            indices.AddRange(new List<int> { t1, t2, t3, t4, t5, t6 });
        } 

        surfaceArray[(int)Mesh.ArrayType.Vertex] = verts.ToArray();
        surfaceArray[(int)Mesh.ArrayType.Index] = indices.ToArray();
        surfaceArray[(int)Mesh.ArrayType.Normal] = normals.ToArray();


        RoadArrayMesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, surfaceArray);
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

        foreach (Vector3 point in m_vertsP1)
        {
            MeshInstance3D mi = new MeshInstance3D();
            BoxMesh boxMesh = new BoxMesh();
            mi.Mesh = boxMesh;
            boxMesh.Size = new Vector3(0.1f, 0.1f, 0.1f);
            mi.Position = point;
            debugInstances.Add(mi);
            AddChild(mi);
        }

        foreach (Vector3 point in m_vertsP2)
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


    public override void _Process(double delta)
    {

    }


}
