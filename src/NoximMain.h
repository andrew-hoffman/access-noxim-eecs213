/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the top-level of Noxim
 */

#ifndef __NOXIMMAIN_H__
#define __NOXIMMAIN_H__

#include <cassert>
#include <systemc.h>
#include <vector>

/* parameter definition of traffic-thermal co-sim */
//#include "define.h"
#include "co-sim.h"

using namespace std;

// Define the directions as numbers
#define DIRECTIONS                   6
#define DIRECTION_NORTH              0
#define DIRECTION_EAST               1
#define DIRECTION_SOUTH              2
#define DIRECTION_WEST               3
#define DIRECTION_UP                 4 
#define DIRECTION_DOWN               5 
#define DIRECTION_LOCAL              6
#define DIRECTION_SEMI_LOCAL         7 //used for redirective pkt 
                                     
#define RCA_LOCAL_RATIO              0.3 //Traffic information based on local or remote
                                         //0.7 for OE3D-version RCA
                                         //0.8 optimization
#define RCA_1D			             1   //1D mode = 0, Fanin mode = 1
#define RCA_QUADRO                   0
// Generic not reserved resource
#define NOT_RESERVED                 -2
// To mark invalid or non exhistent values
#define NOT_VALID                    -1

// Routing algorithms
#define ROUTING_XYZ                  0
#define ROUTING_WEST_FIRST           1
#define ROUTING_NORTH_LAST           2
#define ROUTING_NEGATIVE_FIRST       3
#define ROUTING_ODD_EVEN             4
#define ROUTING_DYAD                 5
#define ROUTING_FULLY_ADAPTIVE       8
#define ROUTING_TABLE_BASED          9
#define ROUTING_PROPOSED             6
#define ROUTING_ZXY                  10
#define ROUTING_DOWNWARD	         11
#define ROUTING_WEST_FIRST_DOWNWARD  12
#define ROUTING_ODD_EVEN_DOWNWARD    13
#define ROUTING_ODD_EVEN_3D          14
#define ROUTING_ODD_EVEN_Z           15
#define ROUTING_DLADR                16
#define ROUTING_DLAR                 17
#define ROUTING_DLDR                 18
#define ROUTING_DOWNWARD_CROSS_LAYER 19
#define INVALID_ROUTING              -1

// Selection strategies
#define SEL_RANDOM                   0
#define SEL_BUFFER_LEVEL             1
#define SEL_NOP                      2
#define SEL_RCA                      3
#define SEL_PROPOSED                 4
#define SEL_OBLIVIOUS                5
#define SEL_THERMAL                  6
#define INVALID_SELECTION            -1

// Traffic distribution              
#define TRAFFIC_RANDOM               0
#define TRAFFIC_TRANSPOSE1           1
#define TRAFFIC_TRANSPOSE2           2
#define TRAFFIC_HOTSPOT              3
#define TRAFFIC_TABLE_BASED          4
#define TRAFFIC_BIT_REVERSAL         5
#define TRAFFIC_SHUFFLE              6
#define TRAFFIC_BUTTERFLY            7
#define INVALID_TRAFFIC              -1
// Input Selection                   
#define INSEL_RANDOM                 0
#define INSEL_CS                     1
// Verbosity levels                  
#define VERBOSE_OFF                  0
#define VERBOSE_LOW                  1
#define VERBOSE_MEDIUM               2
#define VERBOSE_HIGH                 3

/****************MODIFY BY HUI-SHUN********************/
#define DW_BL                        0  //Bottom    Downward Layer
#define DW_ODWL                      1  //Oblivious Downward Layer
#define DW_ADWL                      2  //Adaptive  Downward Lyaer
#define DW_IPD                       3  //Increasing Path Diversity
#define DW_VBDR                      4  //Vertical Balanced Downward Routing
#define DW_ODWL_IPD                  5  //Oblivious Downward Layer + Increasing Path Diversity
/***************THROTTLING MECHANISM*******************/
#define THROT_NORMAL		         0
#define THROT_GLOBAL		         1
#define THROT_DISTRIBUTED	         2
#define THROT_VERTICAL		         3
#define THROT_TEST			         4
#define THROT_TAVT                   5
#define THROT_VERTICAL_MAX           6
#define THROT_TAVT_MAX               7
#define THROT_DYNAMIC                8
#define INVALID_THROT                9
/*** VERTICAL LINK***/
#define VERTICAL_MESH                0
#define VERTICAL_CROSSBAR            1

//Buffer allocation Uniform:1,2,2,11.
#define DEFAULT_BUFFER_ALLOC	     0
//Lateral
#define BUFFER_L_LAYER_0		     1 
#define BUFFER_L_LAYER_1		     2
#define BUFFER_L_LAYER_2		     2
#define BUFFER_L_LAYER_3		     11
//Up
#define BUFFER_U_LAYER_0		     1 
#define BUFFER_U_LAYER_1		     2
#define BUFFER_U_LAYER_2		     2
#define BUFFER_U_LAYER_3		     11
//Down
#define BUFFER_D_LAYER_0		     1 
#define BUFFER_D_LAYER_1		     2
#define BUFFER_D_LAYER_2		     2
#define BUFFER_D_LAYER_3		     11

// Default configuration can be overridden with command-line arguments
#define DEFAULT_VERBOSE_MODE                     VERBOSE_OFF
#define DEFAULT_TRACE_MODE                       false
#define DEFAULT_TRACE_FILENAME                   ""
#define DEFAULT_MESH_DIM_X                       8
#define DEFAULT_MESH_DIM_Y                       8
#define DEFAULT_MESH_DIM_Z                       4
#define DEFAULT_BUFFER_DEPTH                     16
#define DEFAULT_MAX_PACKET_SIZE                  8
#define DEFAULT_MIN_PACKET_SIZE                  8
#define DEFAULT_ROUTING_ALGORITHM                ROUTING_DLADR
#define DEFAULT_ROUTING_TABLE_FILENAME           ""
#define DEFAULT_SELECTION_STRATEGY               SEL_RANDOM
#define DEFAULT_PACKET_INJECTION_RATE            0.01
#define DEFAULT_PROBABILITY_OF_RETRANSMISSION    0.01
#define DEFAULT_TRAFFIC_DISTRIBUTION             TRAFFIC_RANDOM
#define DEFAULT_TRAFFIC_TABLE_FILENAME           "test_traffic.txt"
#define DEFAULT_RESET_TIME                       1
#define DEFAULT_SIMULATION_TIME                  10000  
#define DEFAULT_STATS_WARM_UP_TIME               DEFAULT_RESET_TIME
#define DEFAULT_DETAILED                         false
#define DEFAULT_DYAD_THRESHOLD                   0.6
#define DEFAULT_MAX_VOLUME_TO_BE_DRAINED         0
/****************MODIFY BY HUI-SHUN********************/
#define DEFAULT_DW_LAYER_SEL                     DW_BL  								
#define DEFAULT_DOWN_LEVEL				             3
#define DEFAULT_THROTTLING_TYPE		        THROT_NORMAL
#define DEFAULT_THROTTLING_RATIO			         0
#define DEFAULT_VERTICAL_LINK               VERTICAL_MESH
#define DEFAULT_CASCADE_NODE                false
#define DEFAULT_BELTWAY                     false
#define DEFAULT_MBELTWAY                    false
#define DEFAULT_SBELTWAY                    false
#define DEFAULT_MCASCADE                    false
#define DEFAULT_CLEAN_STAGE_TIME            2000
#define DEFAULT_CAL_TEMP                    true
//message level
#define DEFAULT_MESSAGE_LEVEL                    true
#define DEFAULT_PACKET_QUEUE_LENGTH         4
//
#define MAX_ID NoximGlobalParams::mesh_dim_x*NoximGlobalParams::mesh_dim_y*NoximGlobalParams::mesh_dim_z - 1
//latency histogram
#define HIST_OFF               0
#define HIST_ON                1

#define DEFAULT_ODWL_RATIO     false 
/****************MODIFY BY HUI-SHUN********************/
// TODO by Fafa - this MUST be removed!!! Use only STL vectors instead!!!
#define MAX_STATIC_DIM 20

// NoximGlobalParams -- used to forward configuration to every sub-block
struct NoximGlobalParams {
    static int                          verbose_mode;
    static int                          trace_mode;
    static char                         trace_filename[128];
    static int                          mesh_dim_x;
    static int                          mesh_dim_y;
	static int                          mesh_dim_z;
    static int                          buffer_depth;
    static int                          min_packet_size;
    static int                          max_packet_size;
    static int                          routing_algorithm;
    static char                         routing_table_filename[128];
    static int                          selection_strategy;
    static float                        packet_injection_rate;
    static float                        probability_of_retransmission;
    static int                          traffic_distribution;
    static char                         traffic_table_filename[128];
    static int                          simulation_time;
    static int                          stats_warm_up_time;
    static int                          rnd_generator_seed;
    static bool                         detailed;
    static vector <pair <int, double> > hotspots;
    static float                        dyad_threshold;
    static unsigned int                 max_volume_to_be_drained;

	static int                          dw_layer_sel;
	static int                          burst_length;
	static int                          down_level;
	static int                          throt_type;
	static float                        throt_ratio;
	static double                       max_temp;
	static char                         max_temp_r[40];
	static bool                         pir_is_local_random;
	static bool                         buffer_alloc;
	static int                          vertical_link;
	static bool                         cascade_node;
	static bool                         Mcascade;
	static int                          Mcascade_step;
	static bool                         beltway;
	static int                          br_sel;
	static float                        beltway_trigger;
	static bool                         Mbeltway;
	static bool                         Sbeltway;
	static int                          Sbeltway_ring;
	static int                          ROC_UP;
	static int                          ROC_DOWN;
	static float                        beltway_ratio;
	static bool                         Log_all_Temp;
	static int                          clean_stage_time;
	static bool                         cal_temp;
	
	static bool                         message_level;
	static int                          dynamic_throt_case;
};

// NoximCoord -- XY coordinates type of the Tile inside the Mesh
class NoximCoord {
  public:
    int x;			// X coordinate
    int y;			// Y coordinate
	int z;          // Z coordinate
	
    inline bool operator ==(const NoximCoord & coord) const {
		return (coord.x == x && coord.y == y && coord.z == z);
}};

// NoximFlitType -- Flit type enumeration
enum NoximFlitType {
    FLIT_TYPE_HEAD, FLIT_TYPE_BODY, FLIT_TYPE_TAIL
};

// NoximPayload -- Payload definition
struct NoximPayload {
    sc_uint<32> data;	// Bus for the data to be exchanged

    inline bool operator ==(const NoximPayload & payload) const {
	return (payload.data == data);
}};

// NoximPacket -- Packet definition
struct NoximPacket {
    int    src_id;
    int    dst_id;
	int    mid_id;          // intermedium node id 
    double timestamp;		// SC timestamp at packet generation
	double timestamp_ni;
	double timestamp_nw;
    int    size;
    int    flit_left;		// Number of remaining flits inside the packet
	int    routing;
	int    DW_layer;
	bool   arr_mid;         //record whether this pkt arrive the intermediate node. 0: No 1: Yes
	bool   beltway;
    
    // Constructors
    NoximPacket() {
		arr_mid   = false;
		beltway   = false;
	}
    NoximPacket(const int s, const int d, const double ts, const int sz) {
	make(s, d, ts, sz);
    }

    void make(const int s, const int d, const double ts, const int sz) {
	src_id    = s;
	dst_id    = d;
	mid_id    = d;
	timestamp = ts;
	size      = sz;
	flit_left = sz;
	routing   = NoximGlobalParams::routing_algorithm;
	DW_layer  = NoximGlobalParams::mesh_dim_z - 1;
	arr_mid   = false;
	beltway   = false;
    }
};

// NoximRouteData -- data required to perform routing
struct NoximRouteData {
    int  current_id;
    int  src_id    ;
    int  dst_id    ;
	int  mid_id    ;
    int  dir_in    ;	// direction from which the packet comes from
	int  routing   ;
	int  DW_layer  ;
	bool arr_mid   ;    //record whether this flit arrive the intermediate node 
};

struct NoximChannelStatus {
    int    free_slots;		// occupied buffer slots
    bool   available;		// 
	bool   throttle;        //Foster
    inline bool operator ==(const NoximChannelStatus & bs) const {
		return (free_slots == bs.free_slots && available == bs.available);
    };
};

// NoximNoP_data -- NoP Data definition
struct NoximNoP_data {
    int sender_id;
    NoximChannelStatus channel_status_neighbor[DIRECTIONS];

    inline bool operator ==(const NoximNoP_data & nop_data) const {
	return (sender_id == nop_data.sender_id &&
		   nop_data.channel_status_neighbor[0] == channel_status_neighbor[0]
		&& nop_data.channel_status_neighbor[1] == channel_status_neighbor[1]
		&& nop_data.channel_status_neighbor[2] == channel_status_neighbor[2]
		&& nop_data.channel_status_neighbor[3] == channel_status_neighbor[3]
		);
    };
};

// NoximFlit -- Flit definition
struct NoximFlit {
    int           src_id     ;
    int           dst_id     ;
	int           mid_id     ;
    NoximFlitType flit_type  ;		// The flit type (FLIT_TYPE_HEAD, FLIT_TYPE_BODY, FLIT_TYPE_TAIL)
    int           sequence_no;		// The sequence number of the flit inside the packet
    NoximPayload  payload    ;		// Optional payload
    double        timestamp  ;		// Unix timestamp at packet generation
	double 		  timestamp_ni;
	double 		  timestamp_nw;
    int           hop_no     ;		// Current number of hops from source to destination
	int           routing_f  ;
	int           DW_layer   ;
	int           waiting_cnt;      //for counting contention waiting time
	bool          arr_mid    ;
	bool          beltway    ;
	
	inline bool operator ==(const NoximFlit & flit) const {
	return (flit.src_id == src_id && flit.dst_id == dst_id
		&& flit.flit_type == flit_type
		&& flit.sequence_no == sequence_no
		&& flit.payload == payload && flit.timestamp == timestamp
		&& flit.hop_no == hop_no);
}};

//
inline int getCurrentCycleNum(){
	return (int)(sc_time_stamp().to_double()/1000/CYCLE_PERIOD);
};

// Output overloading

inline ostream & operator <<(ostream & os, const NoximFlit & flit)
{

    if (NoximGlobalParams::verbose_mode == VERBOSE_HIGH) {

	os << "### FLIT ###" << endl;
	os << "Source Tile[" << flit.src_id << "]" << endl;
	os << "Destination Tile[" << flit.dst_id << "]" << endl;
	os << "Intermediate Tile[" << flit.mid_id << "]" << endl;
	switch (flit.flit_type) {
	case FLIT_TYPE_HEAD:
	    os << "Flit Type is HEAD" << endl;
	    break;
	case FLIT_TYPE_BODY:
	    os << "Flit Type is BODY" << endl;
	    break;
	case FLIT_TYPE_TAIL:
	    os << "Flit Type is TAIL" << endl;
	    break;
	}
	os << "Sequence no. " << flit.sequence_no << endl;
	os << "Payload printing not implemented (yet)." << endl;
	os << "Unix timestamp at packet generation " << flit.
	    timestamp << endl;
	os << "Total number of hops from source to destination is " <<
	    flit.hop_no << endl;
    } else {
	os << "[type: ";
	switch (flit.flit_type) {
	case FLIT_TYPE_HEAD:
	    os << "H";
	    break;
	case FLIT_TYPE_BODY:
	    os << "B";
	    break;
	case FLIT_TYPE_TAIL:
	    os << "T";
	    break;
	}

	os << ", seq: " << flit.sequence_no << ", " << flit.src_id << "-->" << flit.mid_id << "-->" << flit.dst_id << " routing: "<<flit.routing_f<<"]";
    if(flit.arr_mid)
		os <<"M-node:0";
	else
		os <<"M-node:1";
	}

    return os;
}

inline ostream & operator <<(ostream & os,
			     const NoximChannelStatus & status)
{
    char msg;
    if (status.available)
	msg = 'A';
    else
	msg = 'N';
    os << msg << "(" << status.free_slots << ")";
    return os;
}

inline ostream & operator <<(ostream & os, const NoximNoP_data & NoP_data)
{
    os << "      NoP data from [" << NoP_data.sender_id << "] [ ";

    for (int j = 0; j < DIRECTIONS; j++)
	os << NoP_data.channel_status_neighbor[j] << " ";

    cout << "]" << endl;
    return os;
}

inline ostream & operator <<(ostream & os, const NoximCoord & coord)
{
	os << "(" << coord.x << "," << coord.y << "," << coord.z <<")";
    return os;
}

// Trace overloading

inline void sc_trace(sc_trace_file * &tf, const NoximFlit & flit, string & name)
{
    sc_trace(tf, flit.src_id, name + ".src_id");
    sc_trace(tf, flit.dst_id, name + ".dst_id");
    sc_trace(tf, flit.sequence_no, name + ".sequence_no");
    sc_trace(tf, flit.timestamp, name + ".timestamp");
    sc_trace(tf, flit.hop_no, name + ".hop_no");
}

inline void sc_trace(sc_trace_file * &tf, const NoximNoP_data & NoP_data, string & name)
{
    sc_trace(tf, NoP_data.sender_id, name + ".sender_id");
}

inline void sc_trace(sc_trace_file * &tf, const NoximChannelStatus & bs, string & name)
{
    sc_trace(tf, bs.free_slots, name + ".free_slots");
    sc_trace(tf, bs.available, name + ".available");
}

// Misc common functions

inline NoximCoord id2Coord(int id)
{
    NoximCoord coord;

    coord.z = id / (NoximGlobalParams::mesh_dim_x*NoximGlobalParams::mesh_dim_y);////
    coord.y = (id-coord.z*NoximGlobalParams::mesh_dim_x*NoximGlobalParams::mesh_dim_y) /NoximGlobalParams::mesh_dim_x;
    coord.x = (id-coord.z*NoximGlobalParams::mesh_dim_x*NoximGlobalParams::mesh_dim_y)  % NoximGlobalParams::mesh_dim_x;

    assert(coord.x < NoximGlobalParams::mesh_dim_x);
    assert(coord.y < NoximGlobalParams::mesh_dim_y);
	assert(coord.z < NoximGlobalParams::mesh_dim_z);

    return coord;
}

inline int coord2Id(const NoximCoord & coord)
{
	int id = coord.z*NoximGlobalParams::mesh_dim_x*NoximGlobalParams::mesh_dim_y + (coord.y * NoximGlobalParams::mesh_dim_x) + coord.x; 
    assert(id < NoximGlobalParams::mesh_dim_x * NoximGlobalParams::mesh_dim_y * NoximGlobalParams::mesh_dim_z); 
    return id;
}

inline int xyz2Id( int x, int y, int z){
	return z * NoximGlobalParams::mesh_dim_x*NoximGlobalParams::mesh_dim_y + y * NoximGlobalParams::mesh_dim_x + x;
}

inline string MarkFileName( string name ){
	char temperal [20];
	sprintf( temperal, "%d", NoximGlobalParams::routing_algorithm);
	if      ( NoximGlobalParams::routing_algorithm == ROUTING_DLADR ){
		if ( NoximGlobalParams::cascade_node ){
			if( NoximGlobalParams::beltway )
				name =name  + "_TTABR";
			else
				name =name  + "_TAAR";
		}
		else
			name =name  + "_DLADR";
	}
	else if ( NoximGlobalParams::routing_algorithm == ROUTING_DLAR  )
		name =name  + "_DLAR";
	else if ( NoximGlobalParams::routing_algorithm == ROUTING_DLDR  )
		name =name  + "_DLDR";
	else
		name =name  + "_routing-" + temperal;
	
	sprintf( temperal, "%d", NoximGlobalParams::selection_strategy);
	name  = name + "_sel-" + temperal;
	sprintf( temperal, "%d", NoximGlobalParams::dw_layer_sel);
	if      ( NoximGlobalParams::dw_layer_sel == DW_BL    )
		name =name  + "_BL";
	else if ( NoximGlobalParams::dw_layer_sel == DW_ODWL  )
		name =name  + "_ODWL";
	else if ( NoximGlobalParams::dw_layer_sel == DW_ADWL  )
		name =name  + "_ADWL";
	else if ( NoximGlobalParams::dw_layer_sel == DW_IPD   )
		name =name  + "_IPD";
	else if ( NoximGlobalParams::dw_layer_sel == DW_ODWL_IPD   )
		name =name  + "_ODWL_IPD";
	else if ( NoximGlobalParams::dw_layer_sel == DW_VBDR   )
		name =name  + "_VBDR";
	else
	name  = name + "_DW-sel-" + temperal;
	sprintf( temperal, "%f", NoximGlobalParams::packet_injection_rate);
	name = name  + "_pir-" + temperal;
	sprintf( temperal, "%d", NoximGlobalParams::traffic_distribution);
	name = name + "_traffic-" + temperal + +".txt";
	return name;
}

#endif
