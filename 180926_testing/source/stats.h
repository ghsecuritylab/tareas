/*
 * stats.h
 *
 */

#ifndef STATS_H_
#define STATS_H_

#include <stdint.h>


typedef struct
{
	uint8_t Ndata;
	int8_t is_max_outlier;
	int8_t is_min_outlier;
	float mean;
	float variance;
	float min;
	float max;
}dataset_t;

/**
 * Usage:
 * uint8_t data[4] = {1,5,6,7};
 * dataset_t dataset;
 * dataset.Ndata = sizeof(data);
 * get_stats(data,&dataset);
 *
 */
void get_stats(int8_t * data, dataset_t * dataset);

#endif /* STATS_H_ */
