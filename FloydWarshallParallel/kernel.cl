__kernel void floydwarshall(__global int * graph, __const int nodes, __const int k)
{
	int i = get_global_id(0);
	int j = get_global_id(1);

	graph[i * nodes + j] = 12345;
}