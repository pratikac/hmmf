#ifndef __hmmf_h__
#define __hmmf_h__

#include "utils/common.h"
#include "systems/singleint.h"

class Edge;
class Vertex;
class Graph;

class Vertex
{
    public:

        State s;
        
        // prob of best path that ends up here incorporating obs
        float prob_best_path;

        // parent of the best path
        Vertex *prev;
        Vertex *next;

        Edge *best_in;
        Edge *best_out;

        list<Edge *> edges_in;
        list<Edge *> edges_out;
        
        Vertex(State& st);
        ~Vertex(){};
        
        friend class System;
};

class Edge{

    public:
        Vertex *from;
        Vertex *to;
        float transition_prob;
        float transition_time;
        
        Edge(Vertex* f, Vertex* t, float prob, float trans_time);

        Edge reverse(){
            return Edge(this->to, this->from, this->transition_prob, this->transition_time);
        }
        ~Edge()
        {
            //cout<<"called destructor"<<endl;

        };
};

class Graph{

    private:
        System* system;
        
        int obs_interval;
        
        float gamma, gamma_t;
        struct kdtree *state_tree;
        
    public:

        Graph(System& sys);
        ~Graph();
        
        vector<Vertex *> vlist;
        list<Edge *> elist;
        
        unsigned int num_vert;
        list<State> truth;
        list<float> obs_times;
        list<State> obs;
        list<State> best_path;
        list<State> kalman_path;
        
        // graph sanity check
        list< list<State> > monte_carlo_trajectories;
        list<float> monte_carlo_probabilities;

        // graph functions
        unsigned int get_num_vert(){return num_vert; };

        void add_vertex(Vertex *v){
            vlist.push_back(v);
            num_vert++;
        }
        void remove_vertex(Vertex* v);
        void remove_edge(Edge *e);
        
        int insert_into_kdtree(Vertex *v);
        Vertex* nearest_vertex(State s);
        void normalize_edges(Vertex *from);
        
        void print_rrg();
        void plot_graph();
        void plot_trajectory();
        void plot_monte_carlo_trajectories(); 

        // algorithm functions
        
        void iterate();
        void add_sample();
        bool is_edge_free( Edge *etmp);
        int connect_edges(Vertex *v);
        int connect_edges_approx(Vertex *v);

        void propagate_system();
        
        void put_init_samples();
        void update_viterbi( Vertex *v );
        void propagate_viterbi(Vertex* v);
        
        void update_observation_prob(State& yt);
        
        void get_best_path();
        void get_kalman_path();
        
        bool is_everything_normalized();
        int simulate_trajectory();
        
        friend class System;
};


#endif