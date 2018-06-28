#include "main.h"

double body_get_pos(traj_required_info_t* traj_info, int body_id, int cord_id)
{
    return traj_info->d->xpos[body_id*3 + cord_id];
}

void body_pos_difference(traj_required_info_t* traj_info, double* xyz_arr, int body_id_end, int body_id_root)
{
    xyz_arr[0] = body_get_pos(traj_info, body_id_end, 0)
        - body_get_pos(traj_info, body_id_root, 0);
    xyz_arr[1] = body_get_pos(traj_info, body_id_end, 1)
        - body_get_pos(traj_info, body_id_root, 1);
    xyz_arr[2] = body_get_pos(traj_info, body_id_end, 2)
        - body_get_pos(traj_info, body_id_root, 2);
}

void vector3_subtraction(double* xyz_result, double* xyz_curr_end_to_root, double* xyz_target_end_to_root)
{
    xyz_result[0] = xyz_target_end_to_root[0] - xyz_curr_end_to_root[0];
    xyz_result[1] = xyz_target_end_to_root[1] - xyz_curr_end_to_root[1];
    xyz_result[2] = xyz_target_end_to_root[2] - xyz_curr_end_to_root[2];
}

double norm_of_vector3_subtraction(double* xyz1, double* xyz2)
{
    double xyz_result[3];
    vector3_subtraction(xyz_result, xyz1, xyz2);
    return mju_norm(xyz_result, 3);
}

double fwd_kinematics_score(
    traj_required_info_t* traj_info,
    double* xyz_xpos_target, 
    int body_id_end)
{
    double* xyz_xpos_curr_end;

    mj_kinematics(traj_info->m,traj_info->d);   
    xyz_xpos_curr_end = traj_info->d->xpos + body_id_end*3;
    return norm_of_vector3_subtraction(xyz_xpos_curr_end, xyz_xpos_target);
}

void fwd_kinematics_compare_result(
    traj_required_info_t* traj_info,
    double* xyz_xpos_target, 
    int body_id_end,
    double* best_diff,
    int* best_qpos_index,
    int* positive_axis,
    int current_index,
    int is_positive)
{
    double observed_diff;

    observed_diff = fwd_kinematics_score(traj_info,xyz_xpos_target,body_id_end);

    if(observed_diff < *best_diff) 
    {
        *best_diff = observed_diff;
        *best_qpos_index = current_index;
        *positive_axis = is_positive;
    }
}

void better_body_optimizer(
    traj_required_info_t* traj_info,
    double* xyz_xpos_target, 
    int body_id_end)
{
    int i;
    double best_diff;
    double pos_val_before_dx;
    int best_qpos_index = -1;
    int positive_axis = 0;
    double dx;

    best_diff = fwd_kinematics_score(traj_info,xyz_xpos_target,body_id_end);
    dx = 1.93 * 0.01 * best_diff + 0.0005;
    for(i = 15 ; i < CASSIE_QPOS_SIZE; i++)
    {
        pos_val_before_dx = traj_info->d->qpos[i];

        traj_info->d->qpos[i] = pos_val_before_dx + dx;

        fwd_kinematics_compare_result(
            traj_info, 
            xyz_xpos_target, 
            body_id_end,
            &best_diff,
            &best_qpos_index,
            &positive_axis,
            i,
            1);

        traj_info->d->qpos[i] = pos_val_before_dx - dx;

        fwd_kinematics_compare_result(
            traj_info,
            xyz_xpos_target, 
            body_id_end,
            &best_diff,
            &best_qpos_index,
            &positive_axis,
            i,
            0);

        traj_info->d->qpos[i] = pos_val_before_dx;
    }

    if(!positive_axis)
        dx = -dx;
    if(best_qpos_index > 2)
        traj_info->d->qpos[best_qpos_index] += dx;
    
}

int allow_pelvis_to_be_grabbed_and_moved(traj_required_info_t* traj_info, double* xyz_ref)
{
    if(traj_info->pert->active) 
    {
        // printf("selected: %d\n", traj_info->pert->select);
        if(traj_info->pert->select == 1)
        {
            traj_info->d->qpos[0] = traj_info->pert->refpos[0];
            traj_info->d->qpos[1] = traj_info->pert->refpos[1];
            traj_info->d->qpos[2] = traj_info->pert->refpos[2];
            return 0;
        }
        else
        {
            xyz_ref[0] = traj_info->pert->refpos[0];
            xyz_ref[1] = traj_info->pert->refpos[1];
            xyz_ref[2] = traj_info->pert->refpos[2];
            return 1;
        }
    }
    return 0;
}

void traj_foreach_frame(traj_required_info_t* traj_info)
{
    double xyz_xpos_target[3];
    int mod = 0;
    mod = allow_pelvis_to_be_grabbed_and_moved(traj_info,xyz_xpos_target);

    for(int z = 0; mod && z < 100; z++)
        better_body_optimizer(traj_info,
            xyz_xpos_target,
            traj_info->pert->select);

    mj_forward(traj_info->m, traj_info->d);
}

