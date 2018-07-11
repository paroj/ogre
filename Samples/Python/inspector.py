import Ogre
import OgreBites

def main():
    ctx = OgreBites.ApplicationContext("OgreInspector")
    ctx.initApp()
    mesh = Ogre.MeshManager.getSingleton().load("facial.mesh", "General")
    
    print(mesh.getName())
    print("Size")
    print("\t", mesh.getBounds().getSize())
    print("Submeshes")
    for sm in mesh.getSubMeshes():
        print("\t", sm.getMaterialName())
    
    for p in mesh.getPoseList():
        print("\t", p.getName())
    
    if not mesh.hasSkeleton():
        return
    
    skel = mesh.getSkeleton()
    print(skel.getName())
    print("Bones")
    for b in skel.getBones():
        print("\t", b.getName())
    print("Animations")
    for i in range(skel.getNumAnimations()):
        print("\t", skel.getAnimation(i).getName())
    
    #ctx.closeApp()
    
if __name__ == "__main__":
    main()