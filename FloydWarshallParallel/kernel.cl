__kernel void floydwarshall(__global long * graph, __const int nodes, __global long * result)
{
	graph[0] = 12345;
	graph[1] = 12345;
}