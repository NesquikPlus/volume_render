package video;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import com.jogamp.opengl.GLAutoDrawable;
import com.jogamp.opengl.GLEventListener;

import glm.mat._4.Mat4;
import glm.vec._3.Vec3;
import glm.vec._4.Vec4;

public class SceneRenderer implements GLEventListener 
{
	private Camera camera;
	private final float level_of_detail = 0.005f;
	private final int verticesListCapacity = (int) (1.0f/level_of_detail) + 1;
	private ArrayList<Vertex> verticesList = new ArrayList<Vertex>(verticesListCapacity);
	
	Vec3 cubeVerticesWorldSpace[] = 
	{
		new Vec3(-0.5f, -0.5f, -0.5f), //sol alt arka
		new Vec3(-0.5f,  0.5f, -0.5f), //sol ust arka
		new Vec3( 0.5f, -0.5f, -0.5f), //sag alt arka
		new Vec3( 0.5f,  0.5f, -0.5f), //sag ust arka

		new Vec3(-0.5f, -0.5f,  0.5f), //sol alt on
		new Vec3(-0.5f,  0.5f,  0.5f), //sol ust on
		new Vec3( 0.5f, -0.5f,  0.5f), //sag alt on
		new Vec3( 0.5f,  0.5f,  0.5f)  //sag ust on
	};

	Vec3 cubeVerticesTexCoords[] = 
	{
	    new Vec3(-1.0f, -1.0f, -1.0f), //sol alt arka
	    new Vec3(-1.0f,  1.0f, -1.0f), //sol ust arka
	    new Vec3( 1.0f, -1.0f, -1.0f), //sag alt arka
	    new Vec3( 1.0f,  1.0f, -1.0f), //sag ust arka

	    new Vec3(-1.0f, -1.0f,  1.0f), //sol alt on
	    new Vec3(-1.0f,  1.0f,  1.0f), //sol ust on
	    new Vec3( 1.0f, -1.0f,  1.0f), //sag alt on
	    new Vec3( 1.0f,  1.0f,  1.0f)  //sag ust on
	};

	int[][] edges =
	{
	    {0,1},
	    {2,3},
	    {0,2},
	    {1,3},
	    {4,5},
	    {6,7},
	    {4,6},
	    {5,7},
	    {0,4},
	    {1,5},
	    {2,6},
	    {3,7}
	};

	private class Vertex
	{	
		Vertex()
		{
			this.vertexCoord = new Vec3(0,0,0);
			this.texCoord = new Vec3(0,0,0);
		}
		
		Vertex(Vec3 vertexCoord, Vec3 texCoord)
		{
			this.vertexCoord = vertexCoord;
			this.texCoord = texCoord;
		}
		
		Vec3 vertexCoord;
		Vec3 texCoord;
	};
	
	
	boolean isPointBetween(Vec3 p1, Vec3 p2, Vec3 p3)//TODO: Optimize
	{
	    float minX, maxX;
	    float minY, maxY;

	    if(p2.x > p3.x){
	        minX = p3.x;
	        maxX = p2.x;
	    }
	    else{
	        maxX = p3.x;
	        minX = p2.x;
	    }

	    if(minX > p1.x || p1.x > maxX)
	        return false;

	    if(p2.y > p3.y){
	        minY = p3.y;
	        maxY = p2.y;
	    }
	    else{
	        maxY = p3.y;
	        minY = p2.y;
	    }

	    if(minY > p1.y || p1.y > maxY)
	        return false;

	    return true;
	}
	
	private float angleWithPositiveX(Vec3 vec)//TODO: Optimize(pseudo-angle)
	{
	    float result = (float) Math.atan2 (vec.y, vec.x);

	    if(result < 0)
	        result += 2 * 3.141592;

	    return result;
	}
	
	private class CCW_Sort implements Comparator<Vertex>
	{
	    @Override
	    public int compare(Vertex v1, Vertex v2) {
	        return angleWithPositiveX(v1.vertexCoord) > angleWithPositiveX(v2.vertexCoord) ? 1 : -1;
	    }
	};
	
	
	void calculatePlanes()
	{
	    int vertexCount = 8;
	    int edgeCount = 12;
	  
	    Vec3 cubeVerticesViewSpace[] = new Vec3[vertexCount];
	    Mat4 viewMatrix = camera.GetViewMatrix();
	    
	    for(int i=0; i < vertexCount; i++)
	    {
	    	Vec4 tmp = new Vec4();
	    	viewMatrix.mul(new Vec4(cubeVerticesWorldSpace[i],1.0f), tmp);
	    	cubeVerticesViewSpace[i] = new Vec3(tmp);
	    }
	    
	    //Find min max z viewSpaceCubeVertex
	    int minIndex = 0;
	    int maxIndex = 0;

	    for(int i=0; i< vertexCount; i++)
	    {
	        if(cubeVerticesViewSpace[i].z < cubeVerticesViewSpace[minIndex].z){
	            minIndex = i;
	        }

	        if(cubeVerticesViewSpace[i].z > cubeVerticesViewSpace[maxIndex].z){
	            maxIndex = i;
	        }
	    }

	    verticesList.clear();
	    float zValue = cubeVerticesViewSpace[minIndex].z;
	    while(zValue < cubeVerticesViewSpace[maxIndex].z)
	    {
	        //Max 6 intersections possible, capacity 6.
	        ArrayList<Vertex> planeVerticesInitial = new ArrayList<Vertex>(6);
	        
	        //For each plane(zValue) find each edge's intersection point.
	        for(int i=0; i < edgeCount; i++)
	        {
	            Vec3 p1 = cubeVerticesViewSpace[edges[i][0]];
	            Vec3 p2 = cubeVerticesViewSpace[edges[i][1]];
	            Vec3 p1TexCoord = cubeVerticesTexCoords[edges[i][0]];
	            Vec3 p2TexCoord = cubeVerticesTexCoords[edges[i][1]];

	            //r(t) = p1 + (p2-p1) * t
	            //Solve parametric equation of line for zValue, p1.z, p2.z and find t.

	            if(p2.z - p1.z == 0)//No intersection with this edge.
	                continue;

	            float t = (zValue - p1.z)/(p2.z - p1.z);
	            Vec3 vertexCoord = new Vec3(p1.x + (p2.x -p1.x) * t, p1.y + (p2.y -p1.y) * t, zValue);
	            
	            if(isPointBetween(vertexCoord, p1, p2) == false)//No intersection with this edge.
	                continue;

	            Vec3 texCoord = p1TexCoord.add_((p2TexCoord.sub_(p1TexCoord)).mul_(t));
	            Vertex intersectionPoint = new Vertex(vertexCoord, texCoord);
	            planeVerticesInitial.add(intersectionPoint);
	        }

	        //Sort vertices of the plane in planeVerticesInitial ccw.
	        Collections.sort(planeVerticesInitial, new CCW_Sort());

	        //Average vertices in planeVerticesInitial find middle vertex for the plane.
	        Vertex midPoint = new Vertex();
	        for(int i=0; i < planeVerticesInitial.size(); i++)
	        {
	            midPoint.vertexCoord.add(planeVerticesInitial.get(i).vertexCoord);
	            midPoint.texCoord.add(planeVerticesInitial.get(i).texCoord);
	        }
	        
	        midPoint.vertexCoord.div(planeVerticesInitial.size());
	        midPoint.texCoord.div(planeVerticesInitial.size());
	        
	        //Construct triangle fan for the plane, add vertices to float vertexBuffer vector.
	        for(int i=0; i < planeVerticesInitial.size() - 1; i++)
	        {
	            verticesList.add(planeVerticesInitial.get(i));
	            verticesList.add(planeVerticesInitial.get(i+1));
	            verticesList.add(midPoint);
	        }
	        verticesList.add(planeVerticesInitial.get(0));
	        verticesList.add(midPoint);
	        //////////////////////////////////////////////////////////////////////////////////

	        zValue += level_of_detail ;
	    }
	}	

	@Override
	public void display(GLAutoDrawable arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void dispose(GLAutoDrawable arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void init(GLAutoDrawable arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void reshape(GLAutoDrawable arg0, int arg1, int arg2, int arg3, int arg4) {
		// TODO Auto-generated method stub
		
	}
	
}


