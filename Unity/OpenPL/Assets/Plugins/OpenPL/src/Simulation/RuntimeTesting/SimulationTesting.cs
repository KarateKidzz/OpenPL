using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;
using System;

namespace OpenPL
{
    public enum MiddlewarePlatform
    {
        FMOD,
        Wwise
    }

    public class SimulationTesting : MonoBehaviour
    {
        System SystemInstance => RuntimeManager.Instance.System;
        Scene SceneInstance => RuntimeManager.Instance.Scene;

        public GameObject Listener;

        public MiddlewarePlatform middlewarePlatform;

        public FMODUnity.StudioEventEmitter eventEmitter;

        public bool DebugMeshes;
        public bool ShowVoxels;

        void Start()
        {
            AcousticGeometry[] allGameObjects = FindObjectsOfType<AcousticGeometry>();

            for (int i = 0; i < allGameObjects.Length; i++)
            {
                AcousticGeometry currentGameObject = allGameObjects[i];

                if (!currentGameObject)
                {
                    continue;
                }

                MeshFilter mesh = currentGameObject.GetComponent<MeshFilter>();

                if (!mesh)
                {
                    UnityEngine.Debug.LogError("[OpenPL] Acoustic Geometry object found with no mesh. This should not be possible");
                    continue;
                }

                if (!mesh.mesh.isReadable)
                {
                    UnityEngine.Debug.LogWarning("Mesh is not readable. Won't be added to the simulation");
                    continue;
                }

                List<Vector3> Verts = new List<Vector3>();
                mesh.mesh.GetVertices(Verts);
                List<int> Inds = new List<int>();
                mesh.mesh.GetIndices(Inds, 0);

                GCHandle VertHandle = GCHandle.Alloc(Verts.ToArray(), GCHandleType.Pinned);
                GCHandle IndHandle = GCHandle.Alloc(Inds.ToArray(), GCHandleType.Pinned);

                int Index = 0;

                try
                {
                    PLVector pos = mesh.transform.position.ToPLVector();
                    PLQuaternion rot = mesh.transform.rotation.ToPLQuaternion();
                    PLVector scal = mesh.transform.lossyScale.ToPLVector();
                    RESULT MeshResult = SceneInstance.AddMesh(ref pos, ref rot, ref scal, VertHandle.AddrOfPinnedObject(), Verts.Count, IndHandle.AddrOfPinnedObject(), Inds.Count, out Index);
                }
                finally
                {
                    VertHandle.Free();
                    IndHandle.Free();
                }
            }

            RuntimeManager.CheckResult(SceneInstance.FillVoxelsWithGeometry(), "Scene.FillVoxels");

            if (DebugMeshes)
            {
                RuntimeManager.CheckResult(SceneInstance.Debug(), "Scene.Debug");
            }

            listenerLocation = Listener.transform.position;
            SystemInstance.SetListenerPosition(listenerLocation.ToPLVector());

            int Count = 0;
            SceneInstance.GetVoxelsCount(ref Count);

            emitterLocation = eventEmitter.transform.position;

            RuntimeManager.CheckResult(SceneInstance.Simulate(emitterLocation.ToPLVector()), "Scene.Simulate");
            
            listenerEmitterLocation = new Vector3(listenerLocation.x, emitterLocation.y, listenerLocation.z);

            RuntimeManager.CheckResult(SceneInstance.DrawGraph(listenerEmitterLocation.ToPLVector()), "DrawGraph");
            RuntimeManager.CheckResult(SceneInstance.DrawGraph(emitterLocation.ToPLVector()), "DrawGraph");

            if (middlewarePlatform == MiddlewarePlatform.FMOD)
            {
                StartCoroutine(WaitForEventInstance());
            }
        }

        FMOD.DSP reverbDSP;
        Vector3 emitterLocation;
        Vector3 listenerEmitterLocation;
        Vector3 listenerLocation;

        int lastVoxel = -1;

        IEnumerator UpdateSimulation()
        {
            while (eventEmitter && eventEmitter.IsPlaying())
            {
                listenerLocation = Listener.transform.position;
                listenerEmitterLocation = new Vector3(listenerLocation.x, emitterLocation.y, listenerLocation.z);

                int VoxelIndex;
                RuntimeManager.CheckResult(SceneInstance.Encode(listenerEmitterLocation.ToPLVector(), out VoxelIndex), "Encode");
                string DesktopPath = global::System.Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
                string currentIRFilePath = DesktopPath + "/" + VoxelIndex.ToString() + ".wav";
                if (global::System.IO.File.Exists(currentIRFilePath) && lastVoxel == VoxelIndex)
                {
                    yield return new WaitForSeconds(1f);
                    continue;
                }
                UnityEngine.Debug.Log(currentIRFilePath);

                lastVoxel = VoxelIndex;

                int channels, sampleRate;
                float[] ir = WAV.Read(currentIRFilePath, out channels, out sampleRate);

                byte[] array = new byte[ir.Length + 1];
                array[0] = (byte)channels;
                Buffer.BlockCopy(ir, 0, array, 1, ir.Length);

                reverbDSP.setParameterData((int)FMOD.DSP_CONVOLUTION_REVERB.IR, array);

                UnityEngine.Debug.Log("SENT IR");

                yield return new WaitForSeconds(0.2f);
            }

            yield return null;
        }

        IEnumerator WaitForEventInstance()
        {
            yield return new WaitForSeconds(5);

            if (!eventEmitter)
            {
                UnityEngine.Debug.LogError("No event");
                yield break;
            }

            if (eventEmitter.EventInstance.getChannelGroup(out FMOD.ChannelGroup group) == FMOD.RESULT.OK)
            {
                int dspNum = -1;
                group.getNumDSPs(out dspNum);

                for (int i = 0; i < dspNum; i++)
                {
                    FMOD.DSP dsp;
                    if (group.getDSP(i, out dsp) == FMOD.RESULT.OK)
                    {
                        FMOD.DSP_TYPE type;
                        dsp.getType(out type);

                        if (type == FMOD.DSP_TYPE.CONVOLUTIONREVERB)
                        {
                            reverbDSP = dsp;
                            yield return UpdateSimulation();
                        }
                    }
                }
            }
            else
            {
                UnityEngine.Debug.Log("No channel group");
            }
            yield return null;
        }

        void OnDrawGizmos()
        {
            if (!RuntimeManager.Instance)
            {
                return;
            }

            Vector3 simulationSize = RuntimeManager.Instance.simulationSize;
            float VoxelSize = RuntimeManager.Instance.VoxelSize;

            Gizmos.color = Color.white;
            Gizmos.DrawWireCube(Vector3.zero, simulationSize);

            if (RuntimeManager.Instance == null)
            {
                return;
            }

            if (SystemInstance.HasHandle() && SceneInstance.HasHandle())
            {
                int Count = 0;

                SceneInstance.GetVoxelsCount(ref Count);

                if (Count != 0)
                {
                    for (int i = 0; i < Count; i++)
                    {
                        PLVector Location = new PLVector();
                        float Absorpivity = 0.0f;
                        SceneInstance.GetVoxelLocation(ref Location, i);
                        SceneInstance.GetVoxelAbsorpivity(ref Absorpivity, i);
                        if (Absorpivity > 0f)
                        {
                            Gizmos.color = Color.green;
                            Gizmos.DrawWireCube(Location.ToVector3(), new Vector3(VoxelSize, VoxelSize, VoxelSize));
                        }
                        else if (ShowVoxels)
                        {
                            Color color = Color.white;
                            color.a = 0.1f;
                            Gizmos.color = color;
                            Gizmos.DrawWireCube(Location.ToVector3(), new Vector3(VoxelSize, VoxelSize, VoxelSize));
                        }
                    }
                }
            }
        }
    }
}
