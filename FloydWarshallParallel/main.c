/*
Project:		FloydWarshallParallel
Filename:		main.c
Author:			Magnus Tait

This program is a naive OpenCL implementation of the Floyd Warshall Algorithm written in C using
Microsoft Visual Studio 2015.

The program reads an input file defining all edges of a graph and arranges it into a 2D integer
array of all possible paths. A sequential Floyd Warshall algorithm is then performed as a baseline.

An OpenCL parallel impementation is then performed. This involves converting the 2D array into a 1D
array to be passed as an argument to the kernel. Two global work dimensions handle the two inner loops
of the sequential Floyd Warshall Algorithm within the kernal file kernel.cl. The outer loop is handled
by incrementing an integer argument to the kernal and enqueueing a new task within a for loop. The
resulting 1D is then retrieved from the memory buffer, converted to a 2D array and then returned.

The program then compares both arrays for mistakes. Both implementations are timed using clock();

Input files consist of one line with an integer defining the total number of nodes in the graph.
The following lines define the edges of the graph with from node, to node and weight seperated by
space characters.

This implementation is completely naive and cannot handle graphs larger than 1024 nodes. That is something
I will work on in the future.
*/

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

//path to input file. Sample 1000 node directedgraph.txt included in git folder.
#define inputfilename "C:/Users/lordsilly/Documents/Visual Studio 2015/Projects/FloydWarshallParallel/directedgraph.txt"

int nodes = 0;				//global counter for number of nodes in graph

//populates graph array from file
int ** populate(FILE * input) {

	char * line = malloc(30);		//string for lines from file

	fgets(line, 30, input);			//get first number of nodes from first line in file
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

	//strings for edge data
	char * from;
	char * to;
	char * weight;

	int step, i, j, k;								//line string index references

	while (fgets(line, 30, input) != NULL) {		//iterate over file by line
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
		//peel edge data from line
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

		//cast edge data to integers and put into graph
		int from2 = atoi(from) - 1;
		int to2 = atoi(to) - 1;
		int weight2 = atoi(weight);
		graph[from2][to2] = weight2;
		free(from);
		free(to);
		free(weight);
	}
	free(line);
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

int ** openCLFW(int ** graph) {
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

	//load kernel
	fopen_s(&fp, fileName, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	//get platform
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	//create cl objects
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
	memsend = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes * nodes * sizeof(int), NULL, &ret);
	memret = clCreateBuffer(context, CL_MEM_READ_WRITE, nodes * nodes * sizeof(int), NULL, &ret);

	ret = clEnqueueWriteBuffer(command_queue, memsend, CL_TRUE, 0, nodes * nodes * sizeof(int), sendGraph, 0, NULL, NULL);

	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	kernel = clCreateKernel(program, "floydwarshall", &ret);

	//arguments
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memsend);
	ret = clSetKernelArg(kernel, 1, sizeof(int), (void *)&nodes);

	//k iterations
	for (int k = 0; k < nodes; k++) {
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

	return(openclFWGraph);
}

int main(void) {

	//populate graph from file
	FILE * input;
	fopen_s(&input, inputfilename, "r");
	if (input == NULL) {
		printf("No File\n");
		exit(EXIT_FAILURE);
	}
	int ** graph;
	graph = populate(input);
	fclose(input);

	//sequential implementation
	int ** FWGraph = deepcopy(graph);
	printf("Sequential Implementation Started\n");
	clock_t start = clock();
	dumbFW(FWGraph);
	printf("Time Taken: %d\n\n", clock() - start);

	//opencl implementation
	printf("OpenCL Implementation Started\n");
	start = clock();
	int ** OpenCLFWGraph = openCLFW(deepcopy(graph));
	printf("Time Taken: %d\n\n", clock() - start);

	int differences = compare(OpenCLFWGraph, FWGraph);
	printf("Number of differences: %d\n", differences);
	if (differences == 0)printf("All shortest paths calculated correctly\n");

	return 0;
	
}