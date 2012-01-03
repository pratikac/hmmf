#!/usr/bin/python

import sys
from time import *
from numpy import *
from random import *
from math import *
from scipy.spatial import kdtree
from pylab import *

try:
    import cPickle as pickle
except:
    import pickle

NUM_DIM=2
zmin = [0,0]
zmax = [1,1]
init_state = array([0, 1])
init_var = array(diag([10e-2, 10e-2]))
consant_holding_time = 0

def wstd(x,w):
    t = w.sum()
    return (((w*x**2).sum()*t-(w*x).sum()**2)/(t**2-(w**2).sum()))**.5
def fdt(x, u, dt):
    return array(([1, -x[1:]]))*dt
def get_process_var(dt):
    return diag(array([10e-2*dt, 0.1*dt]))
def observation_var():
    return 0.1
def get_holding_time(z, u, bowlr):
    bowlr = bowlr*(zmax[0]-zmin[0])
    dt = 1
    if NUM_DIM >=  2:
        return bowlr*bowlr/(bowlr*linalg.norm(fdt(z,u,dt)) + trace(get_process_var(dt)))
    else:
        # return bowlr*bowlr/(bowlr*linalg.norm(fdt(0,1,dt)) + get_process_var(dt)[0,0])
        return bowlr*bowlr/(bowlr*linalg.norm(fdt(z,u,dt)) + get_process_var(dt)[0,0])

def normal_val(x, mu, var):
    dim = len(mu)
    delx = x -mu;
    if len(delx) >= 2:
        det = sqrt(linalg.det(var))
        toret = 1/pow(2*pi,dim/2.0)/det*exp(-0.5*dot(delx,dot(linalg.inv(var),delx)))
        return toret
    else:
        det = sqrt(var[0][0])
        toret = 1/pow(2*pi,dim/2.0)/det*exp(-0.5*dot(delx,delx)/det/det)
        return toret


class Edge:
    transition_probability = 1
    transition_time = 0
    transition_input = 0;

    def __init__(self, prob, time):
        transition_probability = prob
        transition_time = time
        transition_input = []

class Node:
    

    def __init__(self, is_init=False):
        self.x = []
        self.density = 0
        self.edge_probs = []
        self.edge_times = []
        self.edge_controls = []
        
        if is_init == False:
            self.x = array([zmin[i] + random()*(zmax[i]-zmin[i]) for i in range(NUM_DIM)])
        else:
            self.x = array(init_state)
        # self.density = normal_val(self.x, array(init_state), array(init_var))
        # print self.x
    
class Graph:

    def key(self, z):
        return [(z[i] - zmin[i])/(zmax[i]-zmin[i]) for i in range(NUM_DIM)]

    def __init__(self, vert):
        self.num_vert = vert
        self.bowlr = 2.1*pow(log(self.num_vert)/float(self.num_vert),1/float(NUM_DIM))
        self.delta = 0.1

        self.nodes = []
        self.point = []

        self.tree = []
        self.mydict = []

        self.nodes.append(Node(True))
        for i in range(self.num_vert-1):
            self.nodes.append(Node())
        self.points = [self.key(mynode.x) for mynode in self.nodes]
        self.tree = kdtree.KDTree(self.points)

    def draw_edges(self, n1):
        # clear old edges
        n1.edge_probs = []
        n1.edge_times = []

        # n1 is the key of mydict
        probs = []
        holding_time = get_holding_time(n1.x, 0, self.bowlr)
        for n2 in self.mydict[n1]:
            mu = n1.x + fdt(n1.x, 0, holding_time)
            curr_prob = normal_val(n2.x, mu, 
                                   get_process_var(holding_time))
            probs.append(curr_prob)
            
        tot_prob = sum(probs)
        probs = probs/tot_prob
        probs = list(probs)

        n1.edge_probs.append(probs)
        n1.edge_times.append([holding_time for i in range(len(probs))])

    def connect(self):
        for n1 in self.nodes:
            neighbors = []
            neighbors_index = self.tree.query_ball_point(self.key(n1.x), self.bowlr)
            for n2_index in neighbors_index:
                if n1 != self.nodes[n2_index]:
                    if self.nodes[n2_index].x[0] > n1.x[0]:
                        neighbors.append( self.nodes[n2_index])
            self.mydict.append((n1, neighbors))
        
        self.mydict = dict(self.mydict)
        
        for n1 in self.mydict.keys():
            self.draw_edges(n1)
        print "finished connecting"
        count = 0
        count = count+1
        
    def print_graph(self):

        print self.controls
        for n1 in self.mydict.keys():
            print n1.x
            count = 0
            for n2 in self.mydict[n1]:
                print "\t", n2.x
                print "\t\t ", n1.edge_times[self.mydict[n1].index(n2)], " ", n1.edge_probs[self.mydict[n1].index(n2)]
                count = count + 1
            raw_input()

    def find_closest_index(self, mylist, myvar):
        tmp = [ abs(mylist[i] - myvar) for i in range(len(mylist))]
        return tmp.index(min(tmp))

    def simulate_trajectories(self):
        fig = figure(1)
        trajs = []
        traj_probs = []
        traj_times = []
        max_time = zmax[0]
        for traj_index in range(5000):
            traj_curr = []
            curr_time = 0
            traj_time = []
            node_curr = self.nodes[0]
            traj_prob = normal_val(node_curr.x, array(init_state), array(init_var))
                
            traj_curr.append(list(node_curr.x))
            traj_time.append(curr_time)
            # print "changed node to: ", node_curr.x, " time to: ", curr_time

            while curr_time < max_time:
                 
                tmp_probs = node_curr.edge_probs[0]
                if len(tmp_probs) == 0:
                    break

                cum_probs = cumsum(tmp_probs)
                coin_toss = random()
                next_index = 0
                for i in range(len(cum_probs)):
                    if coin_toss >= cum_probs[i]:
                        next_index = next_index+1
                    else:
                        break
                
                #print len(self.mydict[node_curr]), next_index
                traj_prob = traj_prob * node_curr.edge_probs[0][next_index]
                node_curr = self.mydict[node_curr][next_index]
                curr_time = node_curr.x[0]
                # print "changed node to: ", node_curr.x, " time to: ", curr_time

                traj_curr.append(list(node_curr.x))
                traj_time.append(curr_time)

            to_put = [sublist[1] for sublist in traj_curr]
            while len(to_put) < 100:
                to_put.append(to_put[-1])
                traj_time.append(traj_time[-1])

            trajs.append(to_put)
            traj_probs.append(traj_prob)
            traj_times.append(traj_time)
            
            #plot(traj_time, to_put,'bo-')
            #plot(traj_times,traj_controls,'r--')
        
        trajs = array(trajs)
        traj_probs = array(traj_probs)
        traj_avg = average(trajs, axis=0, weights=traj_probs)
        traj_std = array([wstd(trajs[:,i], traj_probs) for i in range(len(trajs[0,:]))])
        
        grid()
        plot(traj_times[0], traj_avg, 'b-', label='mean')
        plot(traj_times[0], traj_avg-traj_std, 'b--', label='+/- std')
        plot(traj_times[0], traj_avg+traj_std, 'b--')
        plot(linspace(0,max_time,1000), exp(-linspace(0,max_time,1000)), 'r-', label='cont. mean')
        xlabel('t [s]')
        ylabel( 'x(t), xd(t)')
        #plot(u_traj[:,0], u_traj[:,1], 'r-')
        show()

if __name__ == "__main__":
        
    # patch kdtree
    kdtree.node = kdtree.KDTree.node
    kdtree.leafnode = kdtree.KDTree.leafnode
    kdtree.innernode = kdtree.KDTree.innernode

    if len(sys.argv) >=3:
        if sys.argv[1] == 'w':
            tic = clock()
            graph = Graph(int(sys.argv[2]))
            graph.connect()
            
            # graph.print_graph()
            pickle.dump(graph, open('graph.pkl','wb'))

            print clock() - tic, '[s]'
    else:
        graph = pickle.load(open('graph.pkl','rb'))

        graph.simulate_trajectories()
