/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the tile
 */

#ifndef __NOXIMVLINK_H__
#define __NOXIMVLINK_H__

#include <systemc.h>
#include "NoximMain.h"
#include "NoximReservationTable.h"

using namespace std;

SC_MODULE(NoximVLink)
{

    // I/O Ports
    sc_in_clk clock;		                	// The input clock for the tile
    sc_in <bool> reset;	                        // The reset signal for the tile

    sc_out  <NoximFlit>  flit_rx_to_UP[3];	// The input channels
    //flit_rx_to_UP = new sc_out  <NoximFlit>[NoximGlobalParams::mesh_dim_z - 1];

    sc_out  <bool>       req_rx_to_UP[3] ;	// The requests associated with the input channels
    //req_rx_to_UP = new sc_out  <bool>[NoximGlobalParams::mesh_dim_z - 1];
    sc_in   <bool>       ack_rx_to_UP[3] ;	// The outgoing ack signals associated with the input channels
    //ack_rx_to_UP  = new sc_in  <bool>[NoximGlobalParams::mesh_dim_z - 1];
    sc_in  <NoximFlit>  flit_tx_to_UP[3];	// The input channels
    //flit_tx_to_UP = new sc_in  <NoximFlit>[NoximGlobalParams::mesh_dim_z - 1];
    sc_in  <bool>       req_tx_to_UP[3] ;	// The requests associated with the input channels
    //req_tx_to_UP = new sc_in  <bool>[NoximGlobalParams::mesh_dim_z - 1];
    sc_out <bool>       ack_tx_to_UP[3] ;	// The outgoing ack signals associated with the input channels
    //ack_tx_to_UP = new sc_out  <bool>[NoximGlobalParams::mesh_dim_z - 1];	
    sc_in  <NoximFlit>  flit_rx_to_DOWN[3];	// The output channels
    ///flit_rx_to_DOWN = new sc_in  <NoximFlit>[NoximGlobalParams::mesh_dim_z - 1];
    sc_in  <bool>       req_rx_to_DOWN[3] ;        // The requests associated with the output channels
    //req_rx_to_DOWN = new sc_in  <bool>[NoximGlobalParams::mesh_dim_z - 1];
    sc_out <bool>       ack_rx_to_DOWN[3] ;	        // The outgoing ack signals associated with the output channels
    //ack_rx_to_DOWN = new sc_out  <bool>[NoximGlobalParams::mesh_dim_z - 1];
    sc_out <NoximFlit>  flit_tx_to_DOWN[3];	// The output channels
    //flit_tx_to_DOWN = new sc_out  <NoximFlit>[NoximGlobalParams::mesh_dim_z - 1];
    sc_out <bool>       req_tx_to_DOWN[3] ;	        // The requests associated with the output channels
    //req_tx_to_DOWN = new sc_out  <bool>[NoximGlobalParams::mesh_dim_z - 1];
    sc_in  <bool>       ack_tx_to_DOWN[3] ;	        // The outgoing ack signals associated with the output channels
    //ack_tx_to_DOWN = new sc_in  <bool>[NoximGlobalParams::mesh_dim_z - 1];

    //Signal
	sc_signal <bool>      req_rx [3];
	sc_signal <bool>      req_tx [3];
	sc_signal <bool>      ack_rx [3];
	sc_signal <bool>      ack_tx [3];
	sc_signal <NoximFlit> flit_rx[3];
	sc_signal <NoximFlit> flit_tx[3];
	//
	void setId( int id){ _id = id;};
	// Constructor
	int i;
    SC_CTOR(NoximVLink) {
	//TO DO
	buildVLink();
	
	SC_METHOD(entry);
	sensitive<<clock.neg();
	}
	
	private:
	NoximReservationTable  reservation_table_UP       ;	 // Switch reservation table
	NoximReservationTable  reservation_table_DOWN     ;	 // Switch reservation table
	void buildVLink   ();
	void entry        ();
	void Mesh         ();
	void CrossBar_UP  ();
	void CrossBar_DOWN();
	int  routingFunction(const NoximRouteData & route_data);
	int  routingDownward(const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination );
	int  routingTLAR    (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination,const int select_routing, int dw_layer);
	int  _id;
	int start_from_layer_U,start_from_layer_D;

};

#endif
