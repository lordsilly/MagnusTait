__kernel void floydwarshall(__global int * graph, __const int nodes, __global int * result)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	if (x < nodes && y < nodes) {
		result[x * nodes + y] = graph[x * nodes + y];
	}
}