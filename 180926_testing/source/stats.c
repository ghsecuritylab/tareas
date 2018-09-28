/*
 * stats.c
 *
 */

#include "stats.h"
#include <math.h>

#define SIGNIFICANCE (3.0f)

static inline float get_mean(int8_t *data, uint8_t Ndata);
static inline float get_variance(int8_t *data, uint8_t Ndata, float mean);
static inline float get_min(int8_t *data, uint8_t Ndata);
static inline float get_max(int8_t *data, uint8_t Ndata);

static inline void is_max_outlier(dataset_t * dataset);
static inline void is_min_outlier(dataset_t * dataset);

void get_stats(int8_t *data, dataset_t * dataset)
{
	dataset->mean = get_mean(data, dataset->Ndata);
	dataset->variance = get_variance(data, dataset->Ndata, dataset->mean);
	dataset->min = get_min(data, dataset->Ndata);
	dataset->max = get_max(data, dataset->Ndata);
	is_max_outlier(dataset);
	is_min_outlier(dataset);
}

static inline float get_mean(int8_t *data, uint8_t Ndata)
{
	float mean = 0;
	uint8_t index;
	for(index = 0 ; index < Ndata ; index++)
	{
		mean += data[index];
	}
	return mean/Ndata;
}

static inline float get_variance(int8_t *data, uint8_t Ndata, float mean)
{
	float variance;
	uint8_t index;
	if(0 == Ndata)
	{
		variance = 0;
	}
	else
	{
		variance = 0;
		for(index = 0 ; index < Ndata ; index++)
		{
			variance += (data[index]-mean)*(data[index]-mean);
		}
		variance/=(Ndata-1);
	}
	return variance;
}

static inline float get_min(int8_t *data, uint8_t Ndata)
{
	int16_t min = 255;
	uint8_t index;
	for(index = 0 ; index < Ndata ; index++)
	{
		if(data[index] < min)
		{
			min = data[index];
		}
	}
	return min;
}

static inline float get_max(int8_t *data, uint8_t Ndata)
{
	int16_t max = 0;
	uint8_t index;
	for(index = 0 ; index < Ndata ; index++)
	{
		if(data[index]> max)
		{
			max = data[index];
		}
	}
	return max;
}

static inline void is_min_outlier(dataset_t *dataset)
{
	dataset->is_min_outlier = 0;
	if(dataset->min < dataset->mean - SIGNIFICANCE*sqrt(dataset->variance))
	{
		dataset->is_min_outlier = 1;
	}
}

static inline void is_max_outlier(dataset_t *dataset)
{
	dataset->is_max_outlier = 0;
	if(dataset->max > dataset->mean + SIGNIFICANCE*sqrt(dataset->variance))
	{
		dataset->is_max_outlier = 1;
	}
}
