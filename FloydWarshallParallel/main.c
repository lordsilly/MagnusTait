#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define inputfilename "C:/Users/lordsilly/Documents/Visual Studio 2015/Projects/FloydWarshallParallel/directedgraph.txt"

int nodes = 0;

//populates graph 2D array from file
int ** populate(FILE * input) {

	char * line = malloc(30);
	fgets(line, 30, input);
	nodes = atoi(line);

	//initialize output graph
	int ** graph = malloc(nodes * sizeof(int*));
	for (int i = 0; i < nodes; i++) {
		graph[i] = malloc(nodes * sizeof(int));
		for (int j = 0; j < nodes; j++) {
			if (i == j)graph[i][j] = 0;				//zero distance to same node
			else graph[i][j] = 999999;				//int max distance to unconnected node
		}
	}

	char * from;
	char * to;
	char * weight;
	int step, i, j, k;
	while (fgets(line, 30, input) != NULL) {
		from = malloc(30);
		to = malloc(30);
		weight = malloc(30);
		for (int i = 0; i < 30; i++) {
			from[i] = '!';
			to[i] = '!';
			weight[i] = '!';
		}
		step = 0;
		i = 0;
		j = 0;
		k = 0;
		for (int l = 0; l < 30; l++) {
			if (line[l] == ' ') { 
				step++; 
			}
			else {
				if (step == 0) {
					from[i] = line[l];
					i++;
				}
				else if (step == 1) {
					to[j] = line[l];
					j++;
				}
				else {
					weight[k] = line[l];
					k++;
				}
			}
		}
		int from2 = atoi(from) - 1;
		int to2 = atoi(to) - 1;
		int weight2 = atoi(weight);
		graph[from2][to2] = weight2;
		free(from);
		free(to);
		free(weight);
		from2 = 0;
		to2 = 0;
		weight2 = 0;
	}
	return graph;
}

//copies a 2d array
int ** deepcopy(int ** input) {
	int ** output = malloc(nodes * sizeof(int*));
	for (int i = 0; i < nodes; i++) {
		output[i] = malloc(nodes * sizeof(int));
		for (int j = 0; j < nodes; j++) {
			output[i][j] = input[i][j];
		}
	}
	return output;
}

//converts 2d array to 1d array
int * twotoone(int ** input) {
	int * output = malloc(nodes * nodes * sizeof(int));
	int i = 0;
	for (int j = 0; j < nodes; j++) {
		for (int k = 0; k < nodes; k++) {
			output[i] = input[j][k];
			i++;
		}
	}
	return output;
}

//converts 1d array to 2d array
int ** onetotwo(int * input) {
	int ** output = malloc(nodes * sizeof(int*));
	output[0] = malloc(nodes * sizeof(int));
	int i = 0;
	int j = 0;
	for (int k = 0; k < (nodes * nodes); k++) {
		output[i][j] = input[k];
		j++;
		if (j == nodes) {
			j = 0;
			i++;
			output[i] = malloc(nodes * sizeof(int*));
		}
	}
	return output;
}

//non-parallel implementation of floyd-warshall algorithm
void dumbFW(int ** graph) {
	for (int k = 0; k < nodes; k++) {
		for (int i = 0; i < nodes; i++) {
			for (int j = 0; j < nodes; j++) {
				if (graph[i][j] > graph[i][k] + graph[k][j]) graph[i][j] = graph[i][k] + graph[k][j];
			}
		}
	}
}

//returns number of differences between two graphs
int compare(int ** a, int ** b) {
	int count = 0;
	for (int i = 0; i < nodes; i++) {
		for (int j = 0; j < nodes; j++) {
			if (a[i][j] != b[i][j]) {
				count++;
			}
		}
	}
	return count;
}

void printGraph(int ** graph) {
	for (int i = 0; i < nodes; i++) {
		printf("%d : ", i);
		for (int j = 0; j < nodes; j++) {
			printf("%d ", graph[i][j]);
		}
		printf("\n");
	}
}

#define MAX_SOURCE_SIZE (0x100000)

int ** openclfw(int ** graph) {
	//setup opencl
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_mem memsend = NULL;
	cl_mem memret = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;

	size_t global[2];
	size_t local[2];

	global[0] = 1024;
	global[1] = 1024;
	local[0] = 32;
	local[1] = 32;

	int * sendGraph = twotoone(graph);
	int * retGraph = malloc(nodes * nodes * sizeof(int));
	FILE *fp;
	char fileName[] = "kernel.cl";
	char *source_str;
	size_t source_size;

	/* Load the source code containing the kernel*/
	fopen_s(&fp, fileName, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	/* Get Platform and Device Info */
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);															/* Create OpenCL context */
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);															/* Create Command Queue */
	memsend = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes * nodes * sizeof(int), NULL, &ret);								/* Create Memory Buffers */
	memret = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes * nodes * sizeof(int), NULL, &ret);

	ret = clEnqueueWriteBuffer(command_queue, memsend, CL_TRUE, 0, nodes * nodes * sizeof(int), sendGraph, 0, NULL, NULL);		// enqueue write buffer

	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);			/* Create Kernel Program from the source */
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);																/* Build Kernel Program */
	kernel = clCreateKernel(program, "floydwarshall", &ret);																	/* Create OpenCL Kernel */

																																//initial arguments, ND Range and first iteration
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memsend);
	ret = clSetKernelArg(kernel, 1, sizeof(int), (void *)&nodes);
	int k = 0;
	ret = clSetKernelArg(kernel, 2, sizeof(int), (void *)&k);
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global, local, 0, NULL, NULL);
	ret = clEnqueueTask(command_queue, kernel, 0, NULL, NULL);

	//k iterations
	for (k = 1; k < nodes; k++) {
		ret = clSetKernelArg(kernel, 2, sizeof(int), (void *)&k);
		ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global, local, 0, NULL, NULL);
		ret = clEnqueueTask(command_queue, kernel, 0, NULL, NULL);
	}

	// get results
	ret = clEnqueueReadBuffer(command_queue, memsend, CL_TRUE, 0, nodes * nodes * sizeof(int), retGraph, 0, NULL, NULL);		/* Copy results from the memory buffer */
	int ** openclFWGraph = onetotwo(retGraph);

	/* Finalization */
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(memsend);
	ret = clReleaseMemObject(memret);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	free(source_str);

	return openclFWGraph;
}

int main(void) {

	//populate graph from file
	FILE * input;
	fopen_s(&input, inputfilename, "r");
	if (input == NULL) {
		printf("No File\n");
		exit(EXIT_FAILURE);
	}
	int ** graph = populate(input);
	fclose(input);

	clock_t time;

	//run sequential implementation
	int ** FWGraph = deepcopy(graph);
	time = clock();
	dumbFW(FWGraph);
	printf("Sequential Time Taken: %d\n", clock() - time);

	//run opencl implementation
	int ** openclFWGraph = deepcopy(graph);
	time = clock();
	openclFWGraph = openclfw(openclFWGraph);
	printf("Sequential Time Taken: %d\n", clock() - time);

	//difference check
	int differences = compare(openclFWGraph, FWGraph);
	printf("Number of differences: %d\n", differences);


	return 0;
	
}