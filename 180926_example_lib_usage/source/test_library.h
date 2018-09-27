/*
 * test_library.h
 *
 *  Created on: Sep 26, 2018
 *      Author: acc
 */

#ifndef TEST_LIBRARY_H_
#define TEST_LIBRARY_H_



typedef enum {success_result,fail_result} test_library_init_results_t;

test_library_init_results_t test_library_init(void);

void test_library_use(void);

#endif /* TEST_LIBRARY_H_ */
