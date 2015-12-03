/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the router
 */

#ifndef __NOXIMROUTER_H__
#define __NOXIMROUTER_H__

#include <systemc.h>
#include "NoximMain.h"
#include "NoximBuffer.h"
#include "NoximStats.h"
#include "NoximGlobalRoutingTable.h"
#include "NoximLocalRoutingTable.h"
#include "NoximReservationTable.h"
using namespace std;

extern unsigned int drained_volume;

SC_MODULE(NoximRouter)
{

    // I/O Ports
    sc_in_clk clock;		                  				// The input clock for the router
    sc_in <bool> reset;                           			// The reset signal for the router

    sc_in  <NoximFlit> flit_rx      [DIRECTIONS + 2];	  	// The input channels (including local one)
    sc_in  <bool     > req_rx       [DIRECTIONS + 2];	  	// The requests associated with the input channels
    sc_out <bool     > ack_rx       [DIRECTIONS + 2];	  	// The outgoing ack signals associated with the input channels
                                    
    sc_out <NoximFlit> flit_tx      [DIRECTIONS + 2];   	// The output channels (including local one)
    sc_out <bool     > req_tx       [DIRECTIONS + 2];	  	// The requests associated with the output channels
    sc_in  <bool     > ack_tx       [DIRECTIONS + 2];	  	// The outgoing ack signals associated with the output channels

    sc_out <int> free_slots         [DIRECTIONS + 1];
    sc_in  <int> free_slots_neighbor[DIRECTIONS + 1];
	             
	sc_out <bool>on_off             [DIRECTIONS];				    // Information to neighbor router that if this router be throttled
	sc_in  <bool>on_off_neighbor    [DIRECTIONS];	                // Information of throttling that if neighbor router be throttled
	
	sc_out <float>TB             [DIRECTIONS];				    // Information to neighbor router TB 
	sc_in  <float>TB_neighbor    [DIRECTIONS];	

	sc_out <float>PDT             [DIRECTIONS];                                  // Information to neighbor router PDT
        sc_in  <float>PDT_neighbor    [DIRECTIONS];

	sc_out <float>buf[DIRECTIONS+2]             [DIRECTIONS];                                  // Information to neighbor router buf0 
        sc_in  <float>buf_neighbor[DIRECTIONS+2]    [DIRECTIONS];	
/*
	sc_out <float>buf[1]             [DIRECTIONS];                                  // Information to neighbor router buf1
        sc_in  <float>buf_neighbor[1]    [DIRECTIONS];

	sc_out <float>buf[2]             [DIRECTIONS];                                  // Information to neighbor router buf2 
        sc_in  <float>buf_neighbor[2]    [DIRECTIONS];

	sc_out <float>buf[3]             [DIRECTIONS];                                  // Information to neighbor router buf3
        sc_in  <float>buf_neighbor[3]    [DIRECTIONS];

	sc_out <float>buf[4]             [DIRECTIONS];                                  // Information to neighbor router buf4 
        sc_in  <float>buf_neighbor[4]    [DIRECTIONS];

	sc_out <float>buf[5]             [DIRECTIONS];                                  // Information to neighbor router buf5 
        sc_in  <float>buf_neighbor[5]    [DIRECTIONS];

	sc_out <float>buf[6]             [DIRECTIONS];                                  // Information to neighbor router buf6 
        sc_in  <float>buf_neighbor[6]    [DIRECTIONS];

	sc_out <float>buf[7]             [DIRECTIONS];                                  // Information to neighbor router buf7 
        sc_in  <float>buf_neighbor[7]    [DIRECTIONS];
*/	
	/*******RCA******/
	// Derek------------------------
	sc_out<int>       RCA_data_out[8];
	sc_in<int>        RCA_data_in[8];	
	//OBL for beltway
	sc_out<int> free_slots_PE[4];
	sc_out<int> RCA_PE[8];
	sc_out<NoximNoP_data> NoP_PE[4];
	
	sc_out<double>	 monitor_out[DIRECTIONS];
	sc_in<double>	 monitor_in [DIRECTIONS];
	
	// Neighbor-on-Path related I/O
    sc_out < NoximNoP_data > NoP_data_out[DIRECTIONS];
    sc_in  < NoximNoP_data > NoP_data_in [DIRECTIONS];
	//
	sc_out < NoximNoP_data > vertical_free_slot_out;
    sc_in  < NoximNoP_data > vertical_free_slot_in[4];
    // Registers
    int                    local_id                ;     // Unique ID
    int                    routing_type            ;     // Type of routing algorithm
    int                    selection_type          ;
    NoximBuffer            buffer[DIRECTIONS + 2]  ;	 // Buffer for each input channel 
    NoximStats             stats                   ;	 // Statistics
    NoximLocalRoutingTable routing_table           ;	 // Routing table
    NoximReservationTable  reservation_table       ;	 // Switch reservation table
    int                    start_from_port         ;	 // Port from which to start the reservation cycle
	int		               cnt_neighbor            ;	 // counter for packets from neighbor routers		
	int		               cnt_received            ;
	double                 buffer_util             ;
	double                 buffer_used             ;
	unsigned int           local_drained           ;
    // Functions  
    unsigned long          getRoutedFlits        ();     // Returns the number of routed flits 
	unsigned long          getRoutedFlits   (int i);     // Returns the number of routed flits
    unsigned long          getRoutedDWFlits      ();     // Returns the number of routed flits 	
	unsigned long          getWaitingTime   (int i);
	unsigned long          getTotalWaitingTime   ();
	unsigned long          getRoutedPackets      ();
    unsigned long          getFlitsCount         (){ return _buffer_pkt_count; };     // Returns the number of flits into the router
	unsigned long          getMsgDelay           (){ return _buffer_pkt_msg_delay; };
	unsigned long          getNiDelay            (){ return _buffer_pkt_ni_delay;  };
	unsigned long          getNwDelay            (){ return _buffer_pkt_nw_delay;  };
	void                   CalcDelay             ();
	int                    getFlitRoute     (int i);
	int                    getDirAvailable  (int i);
    double                 getPower              ();     // Returns the total power dissipated by the router
	void                   TraffThrottlingProcess();
	void                   IntoEmergency         ();
	void                   OutOfEmergency        ();

	void configure(const int _id, const double _warm_up_time,
		   const unsigned int _max_buffer_size,
		   NoximGlobalRoutingTable & grt);
	void TBDB(float consumption_rate);




	void setNoCEmergency( bool noc_emergency){ _noc_emergency = noc_emergency;};
    // Constructor

    SC_CTOR(NoximRouter) {
	SC_METHOD(rxProcess);
	sensitive << reset;
	sensitive << clock.pos();

	SC_METHOD(txProcess);
	sensitive << reset;
	sensitive << clock.pos();

	SC_METHOD(bufferMonitor);
	sensitive << reset;
	sensitive << clock.pos();

	SC_METHOD(RCA_Aggregation);
	sensitive << reset;
	sensitive << clock.pos();

    SC_METHOD(TraffThrottlingProcess);
    sensitive << reset;
    sensitive << clock.pos();

    }

  private:
	
	void                   rxProcess             ();     // The receiving process
    void                   txProcess             ();     // The transmitting process
    void                   bufferMonitor         ();
	
    // performs actual routing + selection
    int route(const NoximRouteData & route_data);
    int Detour(const NoximRouteData & route_data, int input, int waiting);	
    // wrappers
    int           selectionFunction  (const vector <int> &directions, const NoximRouteData & route_data);
    vector < int >routingFunction    (const NoximRouteData & route_data);
	vector < int >DW_layerSelFunction(const int select_routing, const NoximCoord& current, const NoximCoord& destination, const NoximCoord& source, int dw_layer);

    // selection strategies
    int selectionRandom     (const vector <int> & directions                                   );
    int selectionBufferLevel(const vector <int> & directions                                   );
    int selectionNoP        (const vector <int> & directions, const NoximRouteData & route_data);
	int selectionProposed   (const vector <int> & directions, const NoximRouteData & route_data);
    int selectionRCA2D      (const vector <int> & directions, const NoximRouteData & route_data);
    int selectionThermal    (const vector <int> & directions, const NoximRouteData & route_data);	
	// routing functions
	vector < int >routingXYZ             (const NoximCoord & current                          ,const NoximCoord & destination);
	vector < int >routingZXY             (const NoximCoord & current                          ,const NoximCoord & destination);
	vector < int >routingWestFirst       (const NoximCoord & current                          ,const NoximCoord & destination);
    vector < int >routingNorthLast       (const NoximCoord & current                          ,const NoximCoord & destination);
    vector < int >routingNegativeFirst   (const NoximCoord & current                          ,const NoximCoord & destination);
	vector < int >routingLookAhead       (const NoximCoord & current                          ,const NoximCoord & destination);
    vector < int >routingFullyAdaptive   (const NoximCoord & current                          ,const NoximCoord & destination);
    vector < int >routingOddEven         (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination);
    vector < int >routingDyAD            (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination);
	vector < int >routingOddEven_Z       (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination);
	vector < int >routingOddEven_for_3D  (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination);
	vector < int >routingOddEven_3D      (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination);
	vector < int >routingDownward        (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination);
	vector < int >routingWF_Downward     (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination);
	vector < int >routingOddEven_Downward(const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination, const NoximRouteData& route_data); //Foster modified
    vector < int >routingTableBased      (const NoximCoord & current,const int dir_in         ,const NoximCoord & destination);
	vector < int >routingDLADR           (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination,const int select_routing, int dw_layer);  
	vector < int >routingDLAR            (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination,const int select_routing, int dw_layer);  
	vector < int >routingDLDR            (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination,const int select_routing, int dw_layer);  
	vector < int >routingTLAR_DW         (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination); 
	vector < int >routingTLAR_DW_VBDR    (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination); 
	vector < int >routingTLAR_DW_IPD     (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination); 
	vector < int >routingTLAR_DW_ADWL    (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination); 
	vector < int >routingTLAR_DW_ODWL    (const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination, int dw_layer); 
	vector < int >routingTLAR_DW_ODWL_IPD(const NoximCoord & current,const NoximCoord & source,const NoximCoord & destination, int dw_layer); 
	
	unsigned long routed_flits[DIRECTIONS + 2]               ;
	unsigned long waiting	  [DIRECTIONS + 2]               ;
	unsigned long routed_DWflits                             ;
	unsigned long routed_packets                             ;
	vector <NoximFlit> HeadFlit                              ;
	
	
	
    void          RCA_Aggregation                    ()      ;
	bool          inCongestion                       ()      ;
	NoximNoP_data getCurrentNoPData                  () const;
    void          NoP_report                         () const;
    int           reflexDirection       (int direction) const;
    int           getNeighborId(int _id, int direction) const;
	bool          Adaptive_ok(NoximCoord &sour,NoximCoord &dest);
	void          DBA(int outgoing,NoximFlit* head);
    int           NoPScore(const NoximNoP_data & nop_data, const vector <int> & nop_channels) const;
	//Run-time Thermal Management
	bool 	               _emergency               ;     // emergency mode
	int 	               _emergency_level         ;	 // emergency level
	bool	               _throttle_neighbor       ;	 // throttle port from neighbor routers
	unsigned long          _total_waiting           ;
	unsigned long          _buffer_pkt_count        ;
	unsigned long          _buffer_pkt_msg_delay    ;
	unsigned long          _buffer_pkt_ni_delay     ;
	unsigned long          _buffer_pkt_nw_delay     ;
	bool                   _noc_emergency           ;
	unsigned int           RST                      ;
	unsigned int           DFS                      ;
	unsigned int           buf_budget;
    
};

#endif
