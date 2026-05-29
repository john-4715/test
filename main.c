#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	char *average = "1.0000";
	char ave_str[32] = {0};
	unsigned int successful = 279525;
	unsigned int sent = 280295;
	unsigned int errors = 770;

	char success_percent[32] = {0};
	char failure_percent[32] = {0};

	double tmpDoubleVal = successful; // just do data type convert.
	double success_p = (((float)tmpDoubleVal * 1000000) / sent) / 10000.0;
	tmpDoubleVal = errors; // Just do data type convert.
	double failure_p = (((float)tmpDoubleVal * 1000000) / sent) / 10000.0;
	snprintf(success_percent, sizeof(success_percent), "%.4f", success_p);
	snprintf(failure_percent, sizeof(failure_percent), "%.4f", failure_p);

	double dev_average = atof(average);
	snprintf(ave_str, sizeof(ave_str), "%.2f", dev_average);
	dev_average = atof(ave_str);
	printf("ave_str=%s, dev_average=%.2f\n", ave_str, dev_average);
	printf("success_percent=%s,failure_percent=%s\n", success_percent, failure_percent);
	return 0;
}
