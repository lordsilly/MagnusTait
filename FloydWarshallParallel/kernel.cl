__kernel void floydwarshall(__global long * graph, __const int nodes, __global long * result)
{
	for (int i = 0; i < nodes; i++) {
		result[i] = graph[i];
	}
}