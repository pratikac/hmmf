#include "dubins.h"

System::System()
{
    min_states = new double[NUM_DIM];
    max_states = new double[NUM_DIM];
    obs_noise = new double[NUM_DIM];
    process_noise = new double[NUM_DIM];
    init_var = new double[NUM_DIM];

    for(int i=0; i< NUM_DIM; i++)
    {
        min_states[i] = -3;
        max_states[i] = 3;
        init_state.x[i] = 0.1;
    }
    min_states[2] = -M_PI;
    max_states[2] = M_PI;
    init_state.x[2] = 0;

    for(int i=0; i< NUM_DIM; i++)
    {
        process_noise[i] = 1e-3;
        obs_noise[i] = 5e-3;
        init_var[i] = 1e-2;
    }

    sim_time_delta = 0.01;
}

System::~System()
{
    delete[] min_states;
    delete[] max_states;
    delete[] obs_noise;
    delete[] process_noise;
    delete[] init_var;
}

State System::sample()
{
    State s;
    while(1)
    {
        for(int i=0; i< NUM_DIM; i++)
        {
            s.x[i] = min_states[i] + RANDF*( max_states[i] - min_states[i]);
        }

        if( is_free(s) )
            break;
    }
    return s;
}

bool System::is_free(State &s)
{
    return 1;

    bool retflag = 0;

    // obs 1
    if( (s[0] >= 0.127) && (s[0] <= 0.26) )
    {
        if( (s[1] >= 0) && (s[1] <= .217) )
            retflag = 0;
        else
            retflag = 1;
    }
    else
        retflag = 1;

    if (retflag == 0)
        return 0;

    // obs 2
    if( (s[0] >= 0.1) && (s[0] <= 0.2) )
    {
        if( (s[1] >= .32) && (s[1] <= .5) )
            retflag = 0;
        else
            retflag = 1;
    }
    else
        retflag = 1;

    return retflag;
}

int System::get_key(State& s, double *key)
{
    for(int i =0; i < NUM_DIM; i++)
    {
        key[i] = (s.x[i] - min_states[i])/(max_states[i] - min_states[i]);
        //assert(key[i] <= 1.1);
    }
    return 0;
}

State System::integrate(State& s, double duration, bool is_clean)
{
    State t;

    double *var = new double[NUM_DIM];
    double *mean = new double[NUM_DIM];
    double *tmp = new double[NUM_DIM];

    double delta_t = min(duration, 0.01);
    for(int i=0; i<NUM_DIM; i++)
    {
        t.x[i] = s.x[i];
    }

    for(int i=0; i<NUM_DIM; i++)
    {
        var[i] = process_noise[i]*delta_t;
        tmp[i] = 0;
        mean[i] = 0;
    }
        
    double curr_time = 0;
    while(curr_time < duration)
    {
        if( !is_clean)  
            multivar_normal( mean, var, tmp, NUM_DIM);
        
        t.x[0] += (1.0*cos(t.x[2])*delta_t + tmp[0]);
        t.x[1] += (1.0*sin(t.x[2])*delta_t + tmp[1]);
        t.x[2] += (2.0*delta_t + tmp[2]);
        
        if(t.x[2] > M_PI)
            t.x[2] = t.x[2] - 2*M_PI;
        else if(t.x[2] < -M_PI)
            t.x[2] = t.x[2] + 2*M_PI;

        curr_time += min(delta_t, duration - curr_time);
    }

    delete[] mean;
    delete[] tmp;
    delete[] var;

    return t;
}

void System::get_variance(State& s, double duration, double* var)
{
    for(int i=0; i<NUM_DIM; i++)
    {   
        var[i] = process_noise[i]*duration;
    } 
}

void System::get_obs_variance(State& s, double* var)
{
    for(int i=0; i<NUM_DIM_OBS; i++)
    {   
        var[i] = obs_noise[i];
    }
}


State System::observation(State& s, bool is_clean)
{
    State t;

    double *tmp = new double[NUM_DIM_OBS];
    double *mean = new double[NUM_DIM_OBS];

    if( !is_clean)  
        multivar_normal( mean, obs_noise, tmp, NUM_DIM_OBS);

    if(is_clean)
    {
        for(int i=0; i<NUM_DIM_OBS; i++)
            tmp[i] = 0;
    }
    
    double range = sqrt(s.x[0]*s.x[0] + s.x[1]*s.x[1]);
    double heading = atan2(s.x[1], s.x[0]);
    t.x[0] = range + tmp[0];
    t.x[1] = heading + tmp[1];

    delete[] mean;
    delete[] tmp;

    return t;
}

void System::get_kalman_path( vector<State>& obs, vector<double>& obs_times, list<State>& kalman_path, list<State>& kalman_covar)
{

#if 1
    kalman_path.clear();

    kalman_path.push_back(init_state);
    
    Matrix3d Q;
    Q << init_var[0], 0, 0, 0, init_var[1], 0, 0, 0, init_var[2];
    Vector3d curr_state;
    curr_state(0) = init_state.x[0];
    curr_state(1) = init_state.x[1];
    curr_state(2) = init_state.x[2];
    
    Matrix2d Rk = Matrix2d::Zero();
    Rk(0,0)= obs_noise[0];
    Rk(1,1)= obs_noise[1];

    double prev_time = 0;
    for(unsigned int i=0; i< obs.size(); i++)
    {
        State& next_obs = obs[i];
        Vector2d noisy_obs;
        noisy_obs(0) = next_obs.x[0];
        noisy_obs(1) = next_obs.x[1];

        double delta_t = obs_times[i] - prev_time;
        
        //cout<<"delta_t: "<< delta_t << endl;
        State stmp1;
        stmp1.x[0] = curr_state(0);
        stmp1.x[1] = curr_state(1);
        stmp1.x[2] = curr_state(2);
        State next_state = integrate(stmp1, delta_t, true);
        State clean_obs = observation(next_state, true);
        Vector2d obs_vec;
        obs_vec(0) = clean_obs.x[0];
        obs_vec(1) = clean_obs.x[1];

        Matrix3d Ad;
        Ad << 0, 0, -sin(stmp1.x[2]), 0, 0, cos(stmp1.x[2]), 0, 0, 0; 
        
        Matrix3d Wk;
        Wk << process_noise[0]*delta_t, 0, 0, 0, process_noise[1]*delta_t, 0, 0, 0, process_noise[2]*delta_t;
        
        MatrixXd Cd(2,3);
        double range = sqrt(next_state.x[0]*next_state.x[0] + next_state.x[1]*next_state.x[1]);
        Cd(0,0) = next_state.x[0]/range;
        Cd(0,1) = next_state.x[1]/range;
        Cd(0,2) = 0;
        Cd(1,0) = -next_state.x[1]/range/range;
        Cd(1,1) = next_state.x[0]/range/range;
        Cd(1,2) = 0;

        Matrix3d Q_new = Ad * Q * Ad.transpose() + Wk;
        MatrixXd Lk = Q_new*Cd.transpose()*(Cd*Q_new*Cd.transpose() + Rk).inverse();
        
        MatrixXd Sk;
        curr_state(0) = next_state.x[0];
        curr_state(1) = next_state.x[1];
        curr_state(2) = next_state.x[2];
        Sk = noisy_obs - obs_vec;
        
        Vector3d estimate = curr_state + Lk*Sk;
        
        Matrix3d covar = (Matrix3d::Identity() - Lk*Cd)*Q_new;

        Q = covar;
        curr_state = estimate;

        State stmp2;
        stmp2.x[0] = curr_state(0);
        stmp2.x[1] = curr_state(1);
        stmp2.x[2] = curr_state(2);
        kalman_path.push_back(stmp2);
        prev_time = obs_times[i];
    }

#endif
}

