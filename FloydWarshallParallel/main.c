#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define inputfilename "C:/Users/lordsilly/Documents/Visual Studio 2015/Projects/FloydWarshallParallel/directedgraph.txt"

int nodes = 0;

long ** populate(FILE * input) {
	//populates graph array from file

	char * line = malloc(30);
	fgets(line, 30, input);
	nodes = atoi(line);

	//initialize output graph
	long ** graph = malloc(nodes * sizeof(long*));
	for (int i = 0; i < nodes; i++) {
		graph[i] = malloc(nodes * sizeof(long));
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
long ** deepcopy(long ** input) {
	long ** output = malloc(nodes * sizeof(int*));
	for (int i = 0; i < nodes; i++) {
		output[i] = malloc(nodes * sizeof(int));
		for (int j = 0; j < nodes; j++) {
			output[i][j] = input[i][j];
		}
	}
	return output;
}

void dumbFW(long ** graph) {
	for (int k = 0; k < nodes; k++) {
		for (int i = 0; i < nodes; i++) {
			for (int j = 0; j < nodes; j++) {
				if (graph[i][j] > graph[i][k] + graph[k][j]) graph[i][j] = graph[i][k] + graph[k][j];
			}
		}
	}
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

int main(void) {
	//open file
	FILE * input;
	fopen_s(&input, inputfilename, "r");
	if (input == NULL) {
		printf("No File\n");
		exit(EXIT_FAILURE);
	}

	//populate graph
	long ** graph;
	graph = populate(input);
	long ** dumbFWGraph = deepcopy(graph);
	dumbFW(dumbFWGraph);
	printGraph(dumbFWGraph);
}