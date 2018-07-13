#ifndef PDIK_H
#define PDIK_H

#ifndef CASSIE_QPOS_SIZE
#define CASSIE_QPOS_SIZE 35
#endif

#define IK_ITER 100000

#include "mujoco.h"
#include <stdio.h>


struct _pdikdata_t_
{
	mjModel* m;
    mjData* d;
    int32_t doik;
    double lowscore;
    double bestqposes[CASSIE_QPOS_SIZE];
    double initqposes[CASSIE_QPOS_SIZE];
    double target_body[3];
    double target_pelvis[3];
    double target_pelvis_euler[3];
    double target_other[3];
};
typedef struct _pdikdata_t_ pdikdata_t;

void reset_pdikdata(pdikdata_t* ik, mjModel* m, mjData* d);
double apply_pd_controller(double k1, double k2, double* forces, double* xcurr, double* vcurr, double* xtarget);
void pdik_per_step_control(pdikdata_t* t);

#endif

