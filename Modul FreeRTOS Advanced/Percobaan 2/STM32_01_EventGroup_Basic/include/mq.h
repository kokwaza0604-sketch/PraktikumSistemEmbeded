#ifndef MQ_H
#define MQ_H

#include <stdint.h>

/* Read MQ analog sensor on ADC channel (PA0). Returns 0 on success.
 * voltage is in volts, raw is ADC value (0..4095).
 */
int mq_read(float *voltage, uint32_t *raw);

#endif
