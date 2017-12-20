#ifndef COMMON_H
#define COMMON_H

/*
 * Afterwards add a way to fix UDP duplication.
 */
struct sensor_info
{
  int msg_id;
  int node_id;
  float temp;
  float hum;
};

enum role { 
  role_6ln, 
  role_6dr, 
  role_6dr_sec 
};

void print_network_status(void);

#endif /* COMMON_H */