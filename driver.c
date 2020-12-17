#include "driver.h"

driver_t* driver_create(size_t size){
	driver_t* new_driver = (driver_t *)malloc(sizeof(driver_t));
	void** job = (void **)malloc(sizeof(void*));
	new_driver->job = job;
	new_driver->schedule_num = 0;
	new_driver->handle_num = 0;
	if(size == 0){
		new_driver->queued = 0;
		sem_init(&new_driver->mutex, 0, 1);
		sem_init(&new_driver->items, 0, 0);
		sem_init(&new_driver->spaces, 0, 1);
		new_driver->open = 1;
	}
	else{
		new_driver->queued = 1;
		sem_init(&new_driver->mutex, 0, 1);
		sem_init(&new_driver->items, 0, 0);
		sem_init(&new_driver->spaces, 0, (unsigned int) size);
		new_driver->queue = queue_create(size);
		new_driver->open = 1;
	}
	return new_driver;
}

enum driver_status driver_schedule(driver_t *driver, void* job) {
	//sem_wait(&driver->mutex);
	driver->schedule_num += 1;
	//sem_post(&driver->mutex);
	if(driver->queued == 1){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			sem_wait(&driver->spaces);
			if(driver->open == 0){
				sem_post(&driver->spaces);
				return DRIVER_CLOSED_ERROR;
			}
			sem_wait(&driver->mutex);
			queue_add(driver->queue, job);
			sem_post(&driver->mutex);
			sem_post(&driver->items);
			return SUCCESS;
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else if(driver->queued == 0){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			sem_wait(&driver->spaces);
			if(driver->open == 0){
				sem_post(&driver->spaces);
				return DRIVER_CLOSED_ERROR;
			}
			sem_wait(&driver->mutex);
			driver->job[0] = job;
			sem_post(&driver->mutex);
			sem_post(&driver->items);
			return SUCCESS;
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else{
		return DRIVER_GEN_ERROR;
	}
}

enum driver_status driver_handle(driver_t *driver, void **job) {
	//sem_wait(&driver->mutex);
	driver->handle_num += 1;
	//sem_post(&driver->mutex);
	if(driver->queued == 1){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			sem_wait(&driver->items);
			if(driver->open == 0){
				sem_post(&driver->items);
				return DRIVER_CLOSED_ERROR;
			}
			sem_wait(&driver->mutex);
			queue_remove(driver->queue, job);
			sem_post(&driver->mutex);
			sem_post(&driver->spaces);
			return SUCCESS;
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else if(driver->queued == 0){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			sem_wait(&driver->items);
			if(driver->open == 0){
				sem_post(&driver->items);
				return DRIVER_CLOSED_ERROR;
			}
			sem_wait(&driver->mutex);
			*job = driver->job[0];
			sem_post(&driver->mutex);
			sem_post(&driver->spaces);
			return SUCCESS;
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else{
		return DRIVER_GEN_ERROR;
	}
}

enum driver_status driver_non_blocking_schedule(driver_t *driver, void* job) {
	int value = 0;
	int error = 0;
	if(driver->queued == 1){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			if(queue_current_size(driver->queue) == queue_capacity(driver->queue)){
				return DRIVER_REQUEST_FULL;
			}
			else{
				sem_wait(&driver->spaces);
				if(driver->open == 0){
					sem_post(&driver->spaces);
					return DRIVER_CLOSED_ERROR;
				}
				sem_wait(&driver->mutex);
				queue_add(driver->queue, job);
				sem_post(&driver->mutex);
				sem_post(&driver->items);
				return SUCCESS;
			}
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else if(driver->queued == 0){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			if(driver->handle_num >= 1){
				driver->handle_num -= 1;
				error = sem_getvalue(&driver->items, &value);
				if(error == -1){
					return DRIVER_GEN_ERROR;
				}
				if(value == 1){
					return DRIVER_REQUEST_FULL;
				}
				else{
					sem_wait(&driver->spaces);
					if(driver->open == 0){
						sem_post(&driver->spaces);
						return DRIVER_CLOSED_ERROR;
					}
					sem_wait(&driver->mutex);
					driver->job[0] = job;
					sem_post(&driver->mutex);
					sem_post(&driver->items);
					return SUCCESS;
				}
			}
			else if(driver->handle_num == 0){
				return DRIVER_REQUEST_FULL;
			}
			else{
				return DRIVER_GEN_ERROR;
			}
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else{
		return DRIVER_GEN_ERROR;
	}
}

enum driver_status driver_non_blocking_handle(driver_t *driver, void **job) {
	int value = 0;
	int error = 0;
	if(driver->queued == 1){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			if(queue_current_size(driver->queue) == 0){
				return DRIVER_REQUEST_EMPTY;
			}
			else{
				sem_wait(&driver->items);
				if(driver->open == 0){
					sem_post(&driver->items);
					return DRIVER_CLOSED_ERROR;
				}
				sem_wait(&driver->mutex);
				queue_remove(driver->queue, job);
				sem_post(&driver->mutex);
				sem_post(&driver->spaces);
				return SUCCESS;
			}
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else if(driver->queued == 0){
		if(driver->open == 0){
			return DRIVER_CLOSED_ERROR;
		}
		else if(driver->open == 1){
			if(driver->schedule_num >= 1){
				driver->schedule_num -= 1;
				error = sem_getvalue(&driver->items, &value);
				if(error == -1){
					return DRIVER_GEN_ERROR;
				}
				if(value == 0){
					return DRIVER_REQUEST_EMPTY;
				}
				else{
					sem_wait(&driver->items);
					if(driver->open == 0){
						sem_post(&driver->items);
						return DRIVER_CLOSED_ERROR;
					}
					sem_wait(&driver->mutex);
					*job = driver->job[0];
					sem_post(&driver->mutex);
					sem_post(&driver->spaces);
					return SUCCESS;
				}
			}
			else if(driver->schedule_num == 0){
				return DRIVER_REQUEST_EMPTY;
			}
			else{
				return DRIVER_GEN_ERROR;
			}
		}
		else{
			return DRIVER_GEN_ERROR;
		}
	}
	else{
		return DRIVER_GEN_ERROR;
	}
}

enum driver_status driver_close(driver_t *driver) {
	int value = 0;
	int error = 0;

	if(driver->open == 0){
		return DRIVER_GEN_ERROR;
	}

	driver->open = 0;
	error = sem_getvalue(&driver->spaces, &value);

	if(error == -1){
		return DRIVER_GEN_ERROR;
	}
	while(value < 1){
		sem_post(&driver->spaces);
		error = sem_getvalue(&driver->spaces, &value);
		if(error == -1){
			return DRIVER_GEN_ERROR;
		}
	}
	error = sem_getvalue(&driver->items, &value);

	if(error == -1){
		return DRIVER_GEN_ERROR;
	}
	while(value < 1){
		sem_post(&driver->items);
		error = sem_getvalue(&driver->items, &value);
		if(error == -1){
			return DRIVER_GEN_ERROR;
		}
	}
	
	return SUCCESS;
}

enum driver_status driver_destroy(driver_t *driver) {
	if(driver->open == 0){
		if(driver->queued == 1){
			queue_free(driver->queue);
		}
		sem_destroy(&driver->mutex);
		sem_destroy(&driver->items);
		sem_destroy(&driver->spaces);
		free(driver->job);
		free(driver);
		return SUCCESS;
	}
	else if (driver->open == 1){
		return DRIVER_DESTROY_ERROR;
	}
	else{
		return DRIVER_GEN_ERROR;
	}
}

enum driver_status driver_select(select_t *driver_list, size_t driver_count, size_t* selected_index) {
	
	/* IMPLEMENT THIS */
	return 0;
}
