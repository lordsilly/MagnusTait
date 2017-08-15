/*
Project:		FloydWarshallParallel
Filename:		kernel.cl
Author:			Magnus Tait
*/

__kernel void floydwarshall(__global int * graph, __const int nodes, __const int k)
{
	int i = get_global_id(0);
	int j = get_global_id(1);

	if (i < nodes && j < nodes) {
		if (graph[i * nodes + j] > graph[i * nodes + k] + graph[k * nodes + j]) { 
			graph[i * nodes + j] = graph[i * nodes + k] + graph[k * nodes + j];
		}
	}
}