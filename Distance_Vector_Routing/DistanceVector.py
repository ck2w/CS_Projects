# Project 4 for CS 6250: Computer Networks
#
# This defines a DistanceVector (specialization of the Node class)
# that can run the Bellman-Ford algorithm. The TODOs are all related 
# to implementing BF. Students should modify this file as necessary,
# guided by the TODO comments and the assignment instructions. This 
# is the only file that needs to be modified to complete the project.
#
# Student code should NOT access the following members, otherwise they may violate
# the spirit of the project:
#
# topolink (parameter passed to initialization function)
# self.topology (link to the greater topology structure used for message passing)
#
# Copyright 2017 Michael D. Brown
# Based on prior work by Dave Lillethun, Sean Donovan, and Jeffrey Randow.
        											
from Node import *
from helpers import *

class DistanceVector(Node):
    
    def __init__(self, name, topolink, outgoing_links, incoming_links):
        ''' Constructor. This is run once when the DistanceVector object is
        created at the beginning of the simulation. Initializing data structure(s)
        specific to a DV node is done here.'''

        super(DistanceVector, self).__init__(name, topolink, outgoing_links, incoming_links)
        
        #TODO: Create any necessary data structure(s) to contain the Node's internal state / distance vector data    
        self.distance_vector = {self.name: 0}

    def send_initial_messages(self):
        ''' This is run once at the beginning of the simulation, after all
        DistanceVector objects are created and their links to each other are
        established, but before any of the rest of the simulation begins. You
        can have nodes send out their initial DV advertisements here. 

        Remember that links points to a list of Neighbor data structure.  Access
        the elements with .name or .weight '''

        # TODO - Each node needs to build a message and send it to each of its neighbors
        # HINT: Take a look at the skeleton methods provided for you in Node.py
        msg = (self.name, self.distance_vector.copy())
        for link in self.incoming_links:
            self.send_msg(msg, link.name)

    def process_BF(self):
        ''' This is run continuously (repeatedly) during the simulation. DV
        messages from other nodes are received here, processed, and any new DV
        messages that need to be sent to other nodes as a result are sent. '''

        # Implement the Bellman-Ford algorithm here.  It must accomplish two tasks below:
        # TODO 1. Process queued messages
        original_dv = self.distance_vector.copy()
        for msg in self.messages:            
            sender = msg[0]
            for node_name, node_weight in msg[1].items():
                if self.name != node_name:
                    neighbor_weight = int(self.get_outgoing_neighbor_weight(sender)) + node_weight
                else:
                    continue
                if node_weight <= -99:
                    self.distance_vector[node_name] = -99
                elif (node_name not in self.distance_vector) or (neighbor_weight <= self.distance_vector[node_name]):
                    self.distance_vector[node_name] = neighbor_weight
                else:
                    continue
        
        # Empty queue
        self.messages = []

        # TODO 2. Send neighbors updated distances               
        if (self.distance_vector != original_dv):
            msg = (self.name, self.distance_vector)
            for link in self.incoming_links:
                self.send_msg(msg, link.name)
        return

    def log_distances(self):
        ''' This function is called immedately after process_BF each round.  It 
        prints distances to the console and the log file in the following format (no whitespace either end):
        
        A:A0,B1,C2
        
        Where:
        A is the node currently doing the logging (self),
        B and C are neighbors, with vector weights 1 and 2 respectively
        NOTE: A0 shows that the distance to self is 0 '''
        
        # TODO: Use the provided helper function add_entry() to accomplish this task (see helpers.py).
        # An example call that which prints the format example text above (hardcoded) is provided.
        output_string = ','.join('{}{}'.format(k, v) for k, v in self.distance_vector.items())

        add_entry(self.name, output_string)
