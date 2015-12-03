/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the router
 */
#include <math.h>
#include "NoximRouter.h"
#include "NoximStats.h"
extern int throttling[20][20][4];
extern float temp_budget[20][20][4];
extern float MTTT[20][20][4];
extern int wait_cnt[200];

void NoximRouter::rxProcess(){
	int i,j,k;
	if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) 
		cout<<"Router[" << local_id << "]-Rx"<<endl;
    if (reset.read()) {
		// Clear outputs and indexes of receiving protocol
		for ( i = 0; i < DIRECTIONS + 2; i++){ 
			ack_rx[i].write(1);
			routed_flits[i] = 0;
			buffer[i].Clean();
		}
		reservation_table.clear();
		local_drained  = 0;
		routed_DWflits = 0;
		routed_packets = 0;
    } 
	else {
		// For each channel decide if a new flit can be accepted
		// This process simply sees a flow of incoming flits. All arbitration
		// and wormhole related issues are addressed in the txProcess()
	//if(getCurrentCycleNum()%RST == 0)
		for ( i = 0; i < DIRECTIONS + 2; i++) {
			// To accept a new flit, the following conditions must match:
			// 1) there is an incoming request
			// 2) there is a free slot in the input buffer of direction i
			if ( (req_rx[i].read()==1) && ack_rx[i].read()==1 ){
				if ( !_emergency ){//check throttling 
					NoximFlit received_flit = flit_rx[i].read();
					if (NoximGlobalParams::verbose_mode > VERBOSE_OFF  ) {
						cout << getCurrentCycleNum() << ": Router[" << local_id << "], Input[" << i
						<< "], Received flit: " << received_flit << endl;
					}
					received_flit.waiting_cnt = getCurrentCycleNum();
					// Store the incoming flit in the circular buffer
					buffer[i].Push(received_flit);


//					if(received_flit.mid_id == local_id)
//						received_flit.arr_mid = true;

					routed_flits[i]++;
					
					if( received_flit.routing_f != ROUTING_WEST_FIRST )
						routed_DWflits++;
						
					if( received_flit.flit_type == FLIT_TYPE_HEAD )
						routed_packets++;
						
					// Incoming flit
					if ( i != DIRECTION_UP && i != DIRECTION_DOWN)
						stats.power.RouterRxLateral();//Msint + QueuesNDataPath + Clocking
				}
			}
			ack_rx[i].write((!buffer[i].IsFull()) && (!_emergency));
		}
    }
	
	if ( !_emergency )//if router not throttle
		stats.power.TileLeakage();//Router + FPMAC + MEM
	if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) 
		cout<<"Router[" << local_id << "]-Rx ends"<<endl;
}

void NoximRouter::txProcess(){
	if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) 
		cout<<"Router[" << local_id << "]-Tx"<<endl;
    if (reset.read()) {
		// Clear outputs and indexes of transmitting protocol
		for (int i = 0; i < DIRECTIONS + 2; i++){// 0~6 DIRECTIONS + 2 = 8 
			req_tx[i].write(0);
			waiting[i]     = 0;
			}
		_total_waiting  = 0;
		for (int i = 0; i < 200; i++){ 
			wait_cnt[i]     = 0;
		}
    }
	else {
		// 1st phase: Reservation
	//if(getCurrentCycleNum()%RST == 0){
/*
	for (int j = 0; j < DIRECTIONS + 2; j++) {
                        int i = (start_from_port + j) % (DIRECTIONS + 2);
                        int o;
                        if (!buffer[i].IsEmpty()) {
                                NoximFlit flit = buffer[i].Front();
                                if (flit.flit_type == FLIT_TYPE_HEAD && flit.beltway == true) {

                                        NoximRouteData route_data;
                                        route_data.current_id = local_id;
                                        route_data.src_id     = (flit.arr_mid)?flit.mid_id:flit.src_id;
                                        route_data.dst_id     = (flit.arr_mid)?flit.dst_id:flit.mid_id;
                                        route_data.dir_in     = i;
                                        route_data.routing    = flit.routing_f;
                                        route_data.DW_layer   = flit.DW_layer;
                                        route_data.arr_mid    = flit.arr_mid;

                                        o = route(route_data);
                                        if (reservation_table.isAvailable(o) && (getCurrentCycleNum()%DFS)<(4-RST))// && packet_num[i] > 0
                                         {
                                                //if(reservation_table.getOutputPort(i) != o && reservation_table.getOutputPort(i) != NOT_RESERVED)
                                                reservation_table.reserve(i, o);
                                                if (NoximGlobalParams::verbose_mode > VERBOSE_OFF ) {
                                                        cout << getCurrentCycleNum()
                                                        << ": Router[" << local_id
                                                        << "], Input[" << i << "] (" << buffer[i].
                                                        Size() << " flits)" << ", reserved Output["
                                                        << o << "], flit: " << flit << endl;
                                                }
                                                stats.power.ArbiterNControl();
                                        }
                                }
                        }
                }

*/

		for (int j = 0; j < DIRECTIONS + 2; j++) {
			int i = (start_from_port + j) % (DIRECTIONS + 2);
			int o;
			if (!buffer[i].IsEmpty()) {
				NoximFlit flit = buffer[i].Front();	
				if (flit.flit_type == FLIT_TYPE_HEAD) {
					//start_from_port++;
					// prepare data for routing
					NoximRouteData route_data;
					route_data.current_id = local_id;
					route_data.src_id     = (flit.arr_mid)?flit.mid_id:flit.src_id;
					route_data.dst_id     = (flit.arr_mid)?flit.dst_id:flit.mid_id;
					route_data.dir_in     = i;
					route_data.routing    = flit.routing_f;
					route_data.DW_layer   = flit.DW_layer;
					route_data.arr_mid    = flit.arr_mid;
					if( flit.routing_f < 0 ){
						cout<<getCurrentCycleNum()<<":"<<flit<<endl;
						cout<<"flit.current_id"<<"="<<local_id       <<id2Coord(route_data.current_id)<<endl; 
						cout<<"flit.src_id    "<<"="<<flit.src_id    <<id2Coord(flit.src_id          )<<endl;
						cout<<"flit.mid_id    "<<"="<<flit.mid_id    <<id2Coord(flit.mid_id          )<<endl; 		
						cout<<"flit.dst_id    "<<"="<<flit.dst_id    <<id2Coord(flit.dst_id          )<<endl; 
						cout<<"flit.dir_in    "<<"="<<i              <<endl; 
						cout<<"flit.routing   "<<"="<<flit.routing_f <<endl; 
						cout<<"flit.DW_layer  "<<"="<<flit.DW_layer  <<endl; 
						cout<<"flit.arr_mid   "<<"="<<flit.arr_mid   <<endl; 
						assert(false);
					}
					//cout<<endl;
					//cout<<"flit.current_id"<<"="<<local_id       <<id2Coord(route_data.current_id)<<endl;
					//cout<<"flit.DW_layer  "<<"="<<flit.DW_layer  <<endl;
					//cout<<"i:"<<i<<" SFP:"<<start_from_port<<" Waiting time:"<<waiting[i]<<endl;
					if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) 
						cout<<"Before route:"<<flit;
					// Dynamic Beltway Adjustment

//					if(waiting[i] > 100){
//						o = Detour(route_data, i, waiting[i]);
//						if(reservation_table.getOutputPort(i) != NOT_RESERVED)
//							if(o!= reservation_table.getOutputPort(i))
//								reservation_table.release(reservation_table.getOutputPort(i));
//					}
					//cout<<"o:"<<o<<endl;}
//					else{
						o = route(route_data);/*
						if(reservation_table.getOutputPort(i) != NOT_RESERVED)
                                                        if(o!= reservation_table.getOutputPort(i))
                                                                reservation_table.release(reservation_table.getOutputPort(i));*/
//					}
					//if( flit.beltway && !flit.arr_mid)
					//	DBA( o,&buffer[i].Front());//Beltway packet, and in 1st phase
					//

					if (reservation_table.isAvailable(o) && (getCurrentCycleNum()%DFS)<(8-RST))// && packet_num[i] > 0
					 {     
						//if(reservation_table.getOutputPort(i) != o && reservation_table.getOutputPort(i) != NOT_RESERVED)
						reservation_table.reserve(i, o);
						if (NoximGlobalParams::verbose_mode > VERBOSE_OFF ) {
							cout << getCurrentCycleNum()
							<< ": Router[" << local_id
							<< "], Input[" << i << "] (" << buffer[i].
							Size() << " flits)" << ", reserved Output["
							<< o << "], flit: " << flit << endl;
						}
						stats.power.ArbiterNControl();	
					}/*
					else if(waiting[i] > )
					{
						o = Detour(route_data, i, waiting[i]);
					}*/
				}
			}
		}
		start_from_port++;
	
		// 2nd phase: Forwarding
		for(int o=0; o<DIRECTIONS+2; o++)
			if ( 1 == ack_tx[o].read() )req_tx[o].write(0);
			
		for (int i = 0; i < DIRECTIONS + 2; i++) {
			if (!buffer[i].IsEmpty() && !_emergency ) {
				NoximFlit flit = buffer[i].Front();
				int o = reservation_table.getOutputPort(i);

				//if(flit.mid_id == local_id)
				//	flit.arr_mid = true;	

				if (o != NOT_RESERVED) {
					if ( 1 == ack_tx[o].read()) {  //DR input buffer
						if (NoximGlobalParams::verbose_mode > VERBOSE_OFF ) {
							cout << getCurrentCycleNum()
							<< ": Router[" << local_id
							<< "], Input[" << i <<
							"] forward to Output[" << o << "], flit: "
							<< flit << endl;
						}
					if((getCurrentCycleNum()%DFS)<(8-RST)){
						flit_tx[o].write(flit);
						req_tx [o].write(!buffer[i].IsEmpty());
						buffer [i].Pop  ();
						waiting[i] = 0 ;
					
						if( o != DIRECTION_UP && o != DIRECTION_DOWN )
							stats.power.Router2Lateral(); // Crossbar() + Links()				
						
						if (flit.flit_type == FLIT_TYPE_TAIL)
							reservation_table.release(o);
											
						//if ( (flit.flit_type == FLIT_TYPE_HEAD) && ( flit.src_id == 53) && ( flit.dst_id == 25 ) )
						// if ( ( flit.src_id == 14) && ( flit.dst_id == 49 ) )
							_total_waiting += getCurrentCycleNum() - flit.waiting_cnt;

						if (flit.flit_type == FLIT_TYPE_HEAD)
							if(((getCurrentCycleNum() - flit.waiting_cnt) < 200) && (getCurrentCycleNum() > 400000))
								wait_cnt[(getCurrentCycleNum() - flit.waiting_cnt)]++;
														
			
						// Update stats
						if (o == DIRECTION_LOCAL) {
							stats.receivedFlit(getCurrentCycleNum(), flit);
							stats.power.Router2Local();//DualFpmacs + Imem + Dmem + RF + ClockDistribution
							if (NoximGlobalParams::max_volume_to_be_drained) {
								if (drained_volume >= NoximGlobalParams::max_volume_to_be_drained)
									sc_stop();
								else {
									drained_volume++;
									local_drained++;
								}
							}
						}
					 }
					}//there are empty buffer that can receive pkt
					else{//head of line blocking
						waiting[i]++;
					}
				}//output port reserve,
				else{//wait for other input to use that output port
					//count waiting time
					waiting[i]++;
				}
			}
		}
	  //}
    }
}

int NoximRouter::Detour(const NoximRouteData & route_data, int input, int waiting)
{//	assert(false);

	if (route_data.dst_id == local_id && route_data.arr_mid)
                return DIRECTION_LOCAL;
        else if ( (route_data.dst_id == local_id && !route_data.arr_mid))
                return DIRECTION_SEMI_LOCAL; 

        NoximCoord position  = id2Coord(route_data.current_id);
        NoximCoord src_coord = id2Coord(route_data.src_id    );
        NoximCoord dst_coord = id2Coord(route_data.dst_id    );

	vector < int > candidate_channels = routingFullyAdaptive(position, dst_coord);
	//cout<<candidate_channels.size()<<endl;
	for(int i = candidate_channels.size() - 1  ; i >= 0 ; i--){
        	if ( candidate_channels[i] < 4 ) //for lateral direction
                	if ( on_off_neighbor[ candidate_channels[i] ].read() == 1 ){//if the direction is throttled
                               candidate_channels.erase( candidate_channels.begin() + i);//then erase that way 
                        }
                }
	//cout<<candidate_channels.size()<<endl;
	return selectionRandom(candidate_channels); 
}


NoximNoP_data NoximRouter::getCurrentNoPData() const
{
    NoximNoP_data NoP_data;

    for (int j = 0; j < DIRECTIONS; j++) {
	NoP_data.channel_status_neighbor[j].free_slots =
	    free_slots_neighbor[j].read();
	NoP_data.channel_status_neighbor[j].available =
	    (reservation_table.isAvailable(j));
    }
	
    NoP_data.sender_id = local_id;

    return NoP_data;
}

void NoximRouter::bufferMonitor()
{	
	int i;
    if (reset.read()) {
	for ( i = 0; i < DIRECTIONS + 1; i++)
	    free_slots[i].write(buffer[i].GetMaxBufferSize());	
	for(int i=0; i<4; i++)	
		free_slots_PE[i].write( free_slots_neighbor[i]);	
	
	NoximCoord position = id2Coord(local_id);

	if((position.x == 0 && position.y == 0 && position.z == 0) || (position.x == 7 && position.y == 7 && position.z == 7) ||
		(position.x == 7 && position.y == 0 && position.z == 0) ||
		(position.x == 0 && position.y == 7 && position.z == 0) ||
		(position.x == 0 && position.y == 0 && position.z == 7) ||
		(position.x == 7 && position.y == 7 && position.z == 0) ||
		(position.x == 7 && position.y == 0 && position.z == 7) ||
		(position.x == 0 && position.y == 7 && position.z == 7) ){
		buf_budget = 12;
	}else if((position.x > 0 && position.x < 7 && (position.y == 0 && position.z == 0)) || 
		 (position.y > 0 && position.y < 7 && (position.x == 0 && position.z == 0)) || 
		 (position.z > 0 && position.z < 7 && (position.x == 0 && position.y == 0)) ||
		 (position.x > 0 && position.x < 7 && (position.y == 0 && position.z == 7)) ||
                 (position.y > 0 && position.y < 7 && (position.x == 0 && position.z == 7)) || 
                 (position.z > 0 && position.z < 7 && (position.x == 0 && position.y == 7)) ||
		 (position.x > 0 && position.x < 7 && (position.y == 7 && position.z == 0)) ||
                 (position.y > 0 && position.y < 7 && (position.x == 7 && position.z == 0)) || 
                 (position.z > 0 && position.z < 7 && (position.x == 7 && position.y == 0)) ||
		 (position.x > 0 && position.x < 7 && (position.y == 7 && position.z == 7)) ||
                 (position.y > 0 && position.y < 7 && (position.x == 7 && position.z == 7)) || 
                 (position.z > 0 && position.z < 7 && (position.x == 7 && position.y == 7))){
		buf_budget = 8;
	}else if((position.x > 0 && position.x < 7 && position.y > 0 && position.y < 7 && position.z == 0)){
		buf_budget = 4;
	}else
		buf_budget = 0;
	}
	else {
	    // update current input buffers level to neighbors
	    for ( i = 0; i < DIRECTIONS + 1; i++)
			free_slots[i].write(buffer[i].getCurrentFreeSlots());
	    // NoP selection: send neighbor info to each direction 'i'
	    NoximNoP_data current_NoP_data = getCurrentNoPData();

	    for ( i = 0; i < DIRECTIONS; i++)
			NoP_data_out[i].write(current_NoP_data);
		vertical_free_slot_out.write(current_NoP_data);		
		
		for(int i=0; i<4; i++)	
			free_slots_PE[i].write( free_slots_neighbor[i] );
		for(int i=0; i<8; i++)
			RCA_PE[i].write( RCA_data_in[i] );		
		for(int i=0; i<4; i++)		
			NoP_PE[i].write( NoP_data_in[i] );
    }
}

vector < int >NoximRouter::routingFunction(const NoximRouteData & route_data)
{
    NoximCoord position  = id2Coord(route_data.current_id);
    NoximCoord src_coord = id2Coord(route_data.src_id    );
    NoximCoord dst_coord = id2Coord(route_data.dst_id    );
    int dir_in           = route_data.dir_in  ;
	int routing          = route_data.routing ;  
	int DW_layer         = route_data.DW_layer;
	int arr_mid          = route_data.arr_mid ;
	
    switch ( NoximGlobalParams::routing_algorithm ) {
	/***ACCESS IC LAB's Routing Algorithm***/
	case ROUTING_XYZ:
      return routingXYZ(position, dst_coord);
      
    case ROUTING_ZXY:
      return routingZXY(position, dst_coord);
           
    case ROUTING_DOWNWARD:
    	return routingDownward(position, src_coord, dst_coord);
		
	case ROUTING_WEST_FIRST_DOWNWARD:
		if( route_data.routing == ROUTING_WEST_FIRST )
			return routingWestFirst(position, dst_coord);
		else
			return routingWF_Downward(position, src_coord, dst_coord);
			
	case ROUTING_ODD_EVEN_DOWNWARD:
      return routingOddEven_Downward(position, src_coord, dst_coord, route_data);

    case ROUTING_ODD_EVEN_3D:
      return routingOddEven_3D(position, src_coord, dst_coord);

	case ROUTING_ODD_EVEN_Z:
	  return routingOddEven_Z(position, src_coord, dst_coord);

	case ROUTING_WEST_FIRST:
		return routingWestFirst(position, dst_coord);

    case ROUTING_NORTH_LAST:
		return routingNorthLast(position, dst_coord);

    case ROUTING_NEGATIVE_FIRST:
		return routingNegativeFirst(position, dst_coord);

    case ROUTING_ODD_EVEN:
		return routingOddEven(position, src_coord, dst_coord);

    case ROUTING_DYAD:
		return routingDyAD(position, src_coord, dst_coord);

    case ROUTING_FULLY_ADAPTIVE:
		return routingFullyAdaptive(position, dst_coord);

    case ROUTING_TABLE_BASED:
		return routingTableBased( position, dir_in, dst_coord);
	
	case ROUTING_DLADR:
		return routingDLADR( position, src_coord , dst_coord,routing, DW_layer);
	
	case ROUTING_DLAR:
		return routingDLAR( position, src_coord , dst_coord,routing, DW_layer);
	
	case ROUTING_DLDR:
		return routingDLDR( position, src_coord , dst_coord,routing, DW_layer);
    default:
	assert(false);
    }

    // something weird happened, you shouldn't be here
    return (vector < int >) (0);
}

int NoximRouter::route(const NoximRouteData & route_data)
{
    if (route_data.dst_id == local_id && route_data.arr_mid)
		return DIRECTION_LOCAL;
	
	else if ( (route_data.dst_id == local_id && !route_data.arr_mid))
		return DIRECTION_SEMI_LOCAL;
	
	vector < int >candidate_channels = routingFunction(route_data);

	for(int i = candidate_channels.size() - 1  ; i >= 0 ; i--){
			if ( candidate_channels[i] < 4 ) //for lateral direction
				if ( on_off_neighbor[ candidate_channels[i] ].read() == 1 ){//if the direction is throttled
	//				if(candidate_channels.size() == 1)
		//				break;
					candidate_channels.erase( candidate_channels.begin() + i);//then erase that way	
				}
	}

        NoximCoord position = id2Coord(local_id); 

	if( candidate_channels.size() == 0){
		/*
		cout<<"no any candidate channels"<<endl;
		if(!on_off_neighbor[DIRECTION_NORTH] && position.y > 0)//if the direction is not throttled
			candidate_channels.push_back(DIRECTION_NORTH);
		if(!on_off_neighbor[DIRECTION_SOUTH] && position.y < 7)//if the direction is not throttled
                        candidate_channels.push_back(DIRECTION_SOUTH);
		if(!on_off_neighbor[DIRECTION_EAST] && position.x < 7)//if the direction is not throttled
                        candidate_channels.push_back(DIRECTION_EAST);
		if(!on_off_neighbor[DIRECTION_WEST] && position.x > 0)//if the direction is not throttled
                        candidate_channels.push_back(DIRECTION_WEST);*/
		if(!on_off_neighbor[DIRECTION_DOWN]  && position.z < 3)//if the direction is not throttled
			candidate_channels.push_back(DIRECTION_DOWN);
	}
	//For beltway packet, push more dir into candidate_channels to achieve DBA.
    return selectionFunction(candidate_channels, route_data);
}

void NoximRouter::NoP_report() const
{
    NoximNoP_data NoP_tmp;
    cout << getCurrentCycleNum() << ": Router[" << local_id << "] NoP report: " << endl;

    for (int i = 0; i < DIRECTIONS; i++) {
	NoP_tmp = NoP_data_in[i].read();
	if (NoP_tmp.sender_id != NOT_VALID)
	    cout << NoP_tmp;
    }
}

void NoximRouter::RCA_Aggregation()
{
	int i,j;
	if(reset.read()){
		for(i=0; i < DIRECTIONS; i++)
			monitor_out[i].write(0);

		buffer_util = 0;

	}
	else{
	int RCA_data_tmp;
    if( !_emergency ){
		if( NoximGlobalParams::routing_algorithm == ROUTING_ODD_EVEN_Z) {            //Restriction of OE routing is considered
		
		NoximCoord position = id2Coord(local_id);
		
		//N0
		RCA_data_tmp = (buffer[0].getCurrentFreeSlots()*2*32 + RCA_data_in[5].read() + RCA_data_in[6].read())/4;
		//cout << "\tN0: " << RCA_data_tmp;
		RCA_data_out[0].write(RCA_data_tmp);
		//N1
		RCA_data_tmp = (buffer[0].getCurrentFreeSlots()*2*32 + RCA_data_in[4].read() + RCA_data_in[3].read())/4;
		//cout << "\tN1: " << RCA_data_tmp;
		RCA_data_out[1].write(RCA_data_tmp);
		
		//E0
		if(position.x % 2 == 0)
			RCA_data_tmp = (buffer[1].getCurrentFreeSlots()*2*32 + RCA_data_in[7].read() + RCA_data_in[0].read())/4;
		else
			RCA_data_tmp = (buffer[1].getCurrentFreeSlots()*2*32 + RCA_data_in[7].read() + 0)/4;
		//cout << "\tE0: " << RCA_data_tmp;
		RCA_data_out[2].write(RCA_data_tmp);
		//E1
		if(position.x % 2 == 0)
			RCA_data_tmp = (buffer[1].getCurrentFreeSlots()*2*32 + RCA_data_in[6].read() + RCA_data_in[5].read())/4;
		else
			RCA_data_tmp = (buffer[1].getCurrentFreeSlots()*2*32 + RCA_data_in[6].read() + 0)/4;
		//cout << "\tE1: " << RCA_data_tmp;
		RCA_data_out[3].write(RCA_data_tmp);
		
		//S0
		RCA_data_tmp = (buffer[2].getCurrentFreeSlots()*2*32 + RCA_data_in[1].read() + RCA_data_in[2].read())/4;
		//cout << "\tS0: " << RCA_data_tmp;
		RCA_data_out[4].write(RCA_data_tmp);
		//S1
		RCA_data_tmp = (buffer[2].getCurrentFreeSlots()*2*32 + RCA_data_in[0].read() + RCA_data_in[7].read())/4;
		//cout << "\tS1: " << RCA_data_tmp;
		RCA_data_out[5].write(RCA_data_tmp);
		
		//W0
		if(position.x % 2 == 1)
			RCA_data_tmp = (buffer[3].getCurrentFreeSlots()*2*32 + RCA_data_in[3].read() + RCA_data_in[4].read())/4;
		else
			RCA_data_tmp = (buffer[3].getCurrentFreeSlots()*2*32 + RCA_data_in[3].read() + 0)/4;
		//cout << "\tW0: " << RCA_data_tmp;
		RCA_data_out[6].write(RCA_data_tmp);
		//W1
		if(position.x % 2 == 1)
			RCA_data_tmp = (buffer[3].getCurrentFreeSlots()*2*32 + RCA_data_in[2].read() + RCA_data_in[1].read())/4;
		else
			RCA_data_tmp = (buffer[3].getCurrentFreeSlots()*2*32 + RCA_data_in[2].read() + 0)/4;
		//cout << "\tW1: " << RCA_data_tmp;
		RCA_data_out[7].write(RCA_data_tmp);
		}
		else {                              //Restriction of OE routing is NOT considered
		//N0
		RCA_data_tmp = (buffer[0].getCurrentFreeSlots()*2*32 + RCA_data_in[5].read() + RCA_data_in[6].read())/4;
		//cout << "\tN0: " << RCA_data_tmp;
		RCA_data_out[0].write(RCA_data_tmp);
		//N1
		RCA_data_tmp = (buffer[0].getCurrentFreeSlots()*2*32 + RCA_data_in[4].read() + RCA_data_in[3].read())/4;
		//cout << "\tN1: " << RCA_data_tmp;
		RCA_data_out[1].write(RCA_data_tmp);
		//E0
		RCA_data_tmp = (buffer[1].getCurrentFreeSlots()*2*32 + RCA_data_in[7].read() + RCA_data_in[0].read())/4;
		//cout << "\tE0: " << RCA_data_tmp;
		RCA_data_out[2].write(RCA_data_tmp);
		//E1
		RCA_data_tmp = (buffer[1].getCurrentFreeSlots()*2*32 + RCA_data_in[6].read() + RCA_data_in[5].read())/4;
		//cout << "\tE1: " << RCA_data_tmp;
		RCA_data_out[3].write(RCA_data_tmp);
		//S0
		RCA_data_tmp = (buffer[2].getCurrentFreeSlots()*2*32 + RCA_data_in[1].read() + RCA_data_in[2].read())/4;
		//cout << "\tS0: " << RCA_data_tmp;
		RCA_data_out[4].write(RCA_data_tmp);
		//S1
		RCA_data_tmp = (buffer[2].getCurrentFreeSlots()*2*32 + RCA_data_in[0].read() + RCA_data_in[7].read())/4;
		//cout << "\tS1: " << RCA_data_tmp;
		RCA_data_out[5].write(RCA_data_tmp);
		//W0
		RCA_data_tmp = (buffer[3].getCurrentFreeSlots()*2*32 + RCA_data_in[3].read() + RCA_data_in[4].read())/4;
		//cout << "\tW0: " << RCA_data_tmp;
		RCA_data_out[6].write(RCA_data_tmp);
		//W1
		RCA_data_tmp = (buffer[3].getCurrentFreeSlots()*2*32 + RCA_data_in[2].read() + RCA_data_in[1].read())/4;
		//cout << "\tW1: " << RCA_data_tmp;
		RCA_data_out[7].write(RCA_data_tmp);
		}
	}
	else{//in emergency mode
		for( int i = 0 ; i < 8 ; i++)
			RCA_data_out[i].write(0);
	}
		NoximCoord position = id2Coord(local_id);
		double throt_in[DIRECTIONS];
		for(int j=0; j < DIRECTIONS; j++)
			throt_in[j] = monitor_in[j].read();

		int self_throt;
		if(_emergency)
			self_throt = 1;
		else
			self_throt = 0;

		double throt_out[DIRECTIONS];
		for(int j=0; j < DIRECTIONS; j++){
			if((throt_in[j] != 0)||(self_throt != 0)) //OR gate for throttling awareness
				throt_out[j] = 1;
			else
				throt_out[j] = 0;
		}

		monitor_out[DIRECTION_NORTH].write(throt_out[DIRECTION_SOUTH]);
		monitor_out[DIRECTION_SOUTH].write(throt_out[DIRECTION_NORTH]);
		monitor_out[DIRECTION_EAST].write(throt_out[DIRECTION_WEST]);
		monitor_out[DIRECTION_WEST].write(throt_out[DIRECTION_EAST]);

		//cout<<getCurrentCycleNum()<<endl;
		double total_size  = 0;
		buffer_used	= 0;
		for(int j=0; j < 4;j++){
			buffer_used += buffer[j].GetMaxBufferSize() - buffer[j].getCurrentFreeSlots();
			total_size	+= buffer[j].GetMaxBufferSize();
		}
		buffer_util += buffer_used / total_size;
	}
}
//---------------------------------------------------------------------------

int NoximRouter::NoPScore(const NoximNoP_data & nop_data,
			  const vector < int >&nop_channels) const
{
    int score = 0;
	
    for (unsigned int i = 0; i < nop_channels.size(); i++) {
		int available,not_throttle;
		if (nop_data.channel_status_neighbor[nop_channels[i]].available)available = 1;
		else			available = 0;
		int free_slots = nop_data.channel_status_neighbor[nop_channels[i]].free_slots;
		if((nop_channels[i] == DIRECTION_DOWN) || (nop_channels[i] == DIRECTION_UP)){  
			not_throttle = 1;}
		else{
			int b;
			b= (int)monitor_in[i].read();
			if( b == 1 ) not_throttle = 0;
			else         not_throttle = 1;
		}
		score += (int) not_throttle*available*free_slots; //traffic-&throttling-aware
    }

    return score;
}

int NoximRouter::selectionNoP(const vector < int >&directions,
			      const NoximRouteData & route_data)
{
    vector < int >neighbors_on_path;
    vector < int >score;
    int direction_selected = NOT_VALID;
    int current_id         = route_data.current_id;
	
    for ( int i = 0; i < directions.size(); i++) {
	// get id of adjacent candidate
	int candidate_id = getNeighborId(current_id, directions[i]);

	// apply routing function to the adjacent candidate node
	NoximRouteData tmp_route_data;
	tmp_route_data.current_id = candidate_id;
	tmp_route_data.src_id     = route_data.src_id;
	tmp_route_data.dst_id     = route_data.dst_id;
	tmp_route_data.dir_in     = reflexDirection(directions[i]);
	tmp_route_data.DW_layer   = route_data.DW_layer;
	tmp_route_data.routing    = route_data.routing;

	vector < int > next_candidate_channels = routingFunction(tmp_route_data);

	// select useful data from Neighbor-on-Path input 
	NoximNoP_data nop_tmp = NoP_data_in[directions[i]].read();

	// store the score of node in the direction[i]
	score.push_back(NoPScore(nop_tmp, next_candidate_channels));
    }

    // check for direction with higher score
    int  max_direction     = directions[0];
    int  max               = score     [0];
	int  down_score        = 0            ;
	
	for ( int i = 0; i < directions.size(); i++) {
		if (score[i] > max) {
			max_direction = directions[i];
			max           = score     [i];
		}
    }
    // if multiple direction have the same score = max, choose randomly.

    vector < int >equivalent_directions;

    for (unsigned int i = 0; i < directions.size(); i++)
	if (score[i] == max)
	    equivalent_directions.push_back(directions[i]);

    direction_selected =
	equivalent_directions[rand() % equivalent_directions.size()];

    return direction_selected;
}

int NoximRouter::selectionBufferLevel(const vector < int >&directions)
{
    vector < int >best_dirs;
    double max_free_slots  = 0;
    for (unsigned int i = 0; i < directions.size(); i++) {
		double free_slots = free_slots_neighbor[directions[i]].read();
		bool available = reservation_table.isAvailable(directions[i]);
		//double mttt = TB_neighbor[directions[i]].read();
		//double score = TB_neighbor[directions[i]].read() * free_slots;
		if (available) {
			if (free_slots > max_free_slots) {
			max_free_slots = free_slots;
			best_dirs.clear();
			best_dirs.push_back(directions[i]);
			} else if (free_slots == max_free_slots)
			best_dirs.push_back(directions[i]);
		}
    }
    if (best_dirs.size())
	return (best_dirs[rand() % best_dirs.size()]);
    else
	return (directions[rand() % directions.size()]);
}
int NoximRouter::selectionRCA2D(const vector<int>& directions, const NoximRouteData& route_data){
	  vector<int>  best_dirs;
	  int best_RCA_value = 0;
	  
	  if(directions.size()==1) {
		return directions[0];
	  }
	  
	  for(unsigned int i = 0; i < directions.size(); ++i) {
		
		bool available = reservation_table.isAvailable(directions[i]);
		if(available) {
		  int temp_RCA_value;
		  if(directions[i]==DIRECTION_NORTH)
			if(id2Coord(route_data.dst_id).x > id2Coord(route_data.current_id).x)       //NE
			  temp_RCA_value = RCA_data_in[DIRECTION_NORTH*2+1].read();
			else if(id2Coord(route_data.dst_id).x < id2Coord(route_data.current_id).x)  //NW
			  temp_RCA_value = RCA_data_in[DIRECTION_NORTH*2+0].read();
			else                                                                        //N
			  temp_RCA_value = (RCA_data_in[DIRECTION_NORTH*2+1] + RCA_data_in[DIRECTION_NORTH*2+0])/2;
		  else if(directions[i]==DIRECTION_EAST)
			if(id2Coord(route_data.dst_id).y > id2Coord(route_data.current_id).y)       //SE
			  temp_RCA_value = RCA_data_in[DIRECTION_EAST*2+1].read();
			else if(id2Coord(route_data.dst_id).y < id2Coord(route_data.current_id).y)  //NE
			  temp_RCA_value = RCA_data_in[DIRECTION_EAST*2+0].read();
			else                                                                        //E
			  temp_RCA_value = (RCA_data_in[DIRECTION_EAST*2+1] + RCA_data_in[DIRECTION_EAST*2+0])/2;
		  else if(directions[i]==DIRECTION_SOUTH)
			if(id2Coord(route_data.dst_id).x > id2Coord(route_data.current_id).x)       //SE
			  temp_RCA_value = RCA_data_in[DIRECTION_SOUTH*2+0].read();
			else if(id2Coord(route_data.dst_id).x < id2Coord(route_data.current_id).x)  //SW
			  temp_RCA_value = RCA_data_in[DIRECTION_SOUTH*2+1].read();
			else                                                                        //S
			  temp_RCA_value = (RCA_data_in[DIRECTION_SOUTH*2+1] + RCA_data_in[DIRECTION_SOUTH*2+0])/2;
		  else if(directions[i]==DIRECTION_WEST)
			if(id2Coord(route_data.dst_id).y > id2Coord(route_data.current_id).y)       //SW
			  temp_RCA_value = RCA_data_in[DIRECTION_WEST*2+0].read();
			else if(id2Coord(route_data.dst_id).y < id2Coord(route_data.current_id).y)  //NW
			  temp_RCA_value = RCA_data_in[DIRECTION_WEST*2+1].read();
			else                                                                        //W
			  temp_RCA_value = (RCA_data_in[DIRECTION_WEST*2+1] + RCA_data_in[DIRECTION_WEST*2+0])/2;
		  else if(directions[i]==DIRECTION_UP)
			  temp_RCA_value = free_slots_neighbor[DIRECTION_UP]*32;
		  else if(directions[i]==DIRECTION_DOWN)
			  temp_RCA_value = free_slots_neighbor[DIRECTION_DOWN]*32;
		  else
			assert(false);
		  
		  if(temp_RCA_value > best_RCA_value){
			best_RCA_value = temp_RCA_value;
			best_dirs.clear();
			best_dirs.push_back(directions[i]);
		  }
		  else if(temp_RCA_value == best_RCA_value) {
			best_dirs.push_back(directions[i]);
		  }
		}
	  }
	  
	  if(best_dirs.size()) {
		return best_dirs[rand() % best_dirs.size()];
	  }
	  else {
		return directions[rand() % directions.size()];
	  }
	  
	  assert(false);
}
int NoximRouter::selectionProposed(const vector <int> & directions, const NoximRouteData & route_data)
{
	vector < int >neighbors_on_path;
    vector < int >score;
    int direction_selected = NOT_VALID;
    int current_id         = route_data.current_id;
	
    for ( int i = 0; i < directions.size(); i++) {
	// get id of adjacent candidate
	int candidate_id = getNeighborId(current_id, directions[i]);

	// apply routing function to the adjacent candidate node
	NoximRouteData tmp_route_data;
	tmp_route_data.current_id = candidate_id;
	tmp_route_data.src_id     = route_data.src_id;
	tmp_route_data.dst_id     = route_data.dst_id;
	tmp_route_data.dir_in     = reflexDirection(directions[i]);
	tmp_route_data.DW_layer   = route_data.DW_layer;
	tmp_route_data.routing    = route_data.routing;

	vector < int > next_candidate_channels = routingFunction(tmp_route_data);

	// select useful data from Neighbor-on-Path input 
	NoximNoP_data nop_tmp = NoP_data_in[directions[i]].read();

	// store the score of node in the direction[i]
	if( current_id < 128 && i < 4 )
		score.push_back(NoPScore(nop_tmp, next_candidate_channels)*2);
	else
		score.push_back(NoPScore(nop_tmp, next_candidate_channels));
    }

    // check for direction with higher score
    int  max_direction     = directions[0];
    int  max               = score     [0];
	int  down_score        = 0            ;
	
	for ( int i = 0; i < directions.size(); i++) {
		if (score[i] > max) {
			max_direction = directions[i];
			max           = score     [i];
		}
    }
    // if multiple direction have the same score = max, choose randomly.

    vector < int >equivalent_directions;

    for (unsigned int i = 0; i < directions.size(); i++)
	if (score[i] == max)
	    equivalent_directions.push_back(directions[i]);

    direction_selected =
	equivalent_directions[rand() % equivalent_directions.size()];

    return direction_selected;
}

int NoximRouter::selectionThermal(const vector<int>& directions, const NoximRouteData& route_data){ //revised by 20130803  //Thermal delay based

	NoximCoord current = id2Coord(route_data.current_id);
	NoximCoord     dst = id2Coord(route_data.dst_id);

        if(directions.size()==1) {
                return directions[0];
        }        
        else if(directions.size()==2) {
                
                //throttling[packet_dst.x][packet_dst.y][packet_dst.z]

		float thermal_delay0 = 0;
		float thermal_delay1 = 0;
		float BCT0 = free_slots_neighbor[directions[0]].read();
		float BCT1 = free_slots_neighbor[directions[1]].read();

		thermal_delay0 = buf_neighbor[directions[0]][directions[0]].read() + BCT0;
		thermal_delay1 = buf_neighbor[directions[1]][directions[1]].read() + BCT1;
/*
		if(thermal_delay1 < thermal_delay0)
                        return directions[1];
                else
                        return directions[0];
*/		
		// because of throttle , three path desrease to two path
		if(directions[0]==DIRECTION_NORTH && directions[1]==DIRECTION_EAST){
			thermal_delay0 += 0.4*(buf_neighbor[DIRECTION_DOWN][directions[0]].read()+buf_neighbor[DIRECTION_EAST][directions[0]].read()+buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read()) + 0.2*buf_neighbor[DIRECTION_SOUTH][directions[0]].read() + 0.2*buf_neighbor[DIRECTION_WEST][directions[0]].read();
			thermal_delay1 += 0.4*(buf_neighbor[DIRECTION_DOWN][directions[1]].read()+buf_neighbor[DIRECTION_NORTH][directions[1]].read()+buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read()) + 0.2*buf_neighbor[DIRECTION_SOUTH][directions[1]].read() + 0.2*buf_neighbor[DIRECTION_WEST][directions[1]].read();
		}
		else if(directions[0]==DIRECTION_NORTH && directions[1]==DIRECTION_DOWN){
			//buf_neighbor[directions[1]][directions[0]]
			thermal_delay0 +=  buf_neighbor[DIRECTION_EAST][directions[0]].read()/5 + buf_neighbor[DIRECTION_WEST][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/5 + buf_neighbor[DIRECTION_EAST][directions[0]].read()/5 + buf_neighbor[DIRECTION_SOUTH][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/6 + buf_neighbor[DIRECTION_EAST][directions[0]].read()/6 + buf_neighbor[DIRECTION_WEST][directions[0]].read()/6 + buf_neighbor[DIRECTION_SOUTH][directions[0]].read()/6 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read())*17/30;

			thermal_delay1 += (buf_neighbor[DIRECTION_EAST][directions[1]].read()+buf_neighbor[DIRECTION_NORTH][directions[1]].read())*17/30 + (buf_neighbor[DIRECTION_WEST][directions[1]].read()+buf_neighbor[DIRECTION_SOUTH][directions[1]].read())*11/30 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read())*17/30;
		}
		else if(directions[0]==DIRECTION_SOUTH && directions[1]==DIRECTION_EAST){
			thermal_delay0 += 0.4*(buf_neighbor[DIRECTION_DOWN][directions[0]].read()+buf_neighbor[DIRECTION_EAST][directions[0]].read()+buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read()) + 0.2*buf_neighbor[DIRECTION_NORTH][directions[0]].read() + 0.2*buf_neighbor[DIRECTION_WEST][directions[0]].read();
			thermal_delay1 += 0.4*(buf_neighbor[DIRECTION_DOWN][directions[1]].read()+buf_neighbor[DIRECTION_SOUTH][directions[1]].read()+buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read()) + 0.2*buf_neighbor[DIRECTION_NORTH][directions[1]].read() + 0.2*buf_neighbor[DIRECTION_WEST][directions[1]].read();
		}
		else if(directions[0]==DIRECTION_SOUTH && directions[1]==DIRECTION_DOWN){
			thermal_delay0 += (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_EAST][directions[0]].read()+buf_neighbor[DIRECTION_DOWN][directions[0]].read())*17/30 + (buf_neighbor[DIRECTION_NORTH][directions[0]].read()+buf_neighbor[DIRECTION_WEST][directions[0]].read())*11/30;

			thermal_delay1 += (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_EAST][directions[1]].read()+buf_neighbor[DIRECTION_SOUTH][directions[1]].read())*17/30+ (buf_neighbor[DIRECTION_NORTH][directions[1]].read()+buf_neighbor[DIRECTION_WEST][directions[1]].read())*11/30;
                }
		else if(directions[0]==DIRECTION_EAST && directions[1]==DIRECTION_DOWN){
			thermal_delay0 += (buf_neighbor[DIRECTION_EAST][directions[0]].read()/5 + buf_neighbor[DIRECTION_WEST][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/5 + buf_neighbor[DIRECTION_EAST][directions[0]].read()/5 + buf_neighbor[DIRECTION_SOUTH][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/6 + buf_neighbor[DIRECTION_EAST][directions[0]].read()/6 + buf_neighbor[DIRECTION_WEST][directions[0]].read()/6 + buf_neighbor[DIRECTION_SOUTH][directions[0]].read()/6 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read())*17/30 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_EAST][directions[0]].read()+buf_neighbor[DIRECTION_DOWN][directions[0]].read())*17/30 + (buf_neighbor[DIRECTION_NORTH][directions[0]].read()+buf_neighbor[DIRECTION_WEST][directions[0]].read())*11/30)/2;

			thermal_delay1 += ((buf_neighbor[DIRECTION_EAST][directions[1]].read()+buf_neighbor[DIRECTION_NORTH][directions[1]].read())*17/30 + (buf_neighbor[DIRECTION_WEST][directions[1]].read()+buf_neighbor[DIRECTION_SOUTH][directions[1]].read())*11/30 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read())*17/30 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_EAST][directions[1]].read()+buf_neighbor[DIRECTION_SOUTH][directions[1]].read())*17/30+ (buf_neighbor[DIRECTION_NORTH][directions[1]].read()+buf_neighbor[DIRECTION_WEST][directions[1]].read())*11/30)/2;
		}
	//PTMAR
		
		if(thermal_delay1 < thermal_delay0)
                        return directions[1];
                else
                        return directions[0];
	

        }else if(directions.size()==3){

		float thermal_delay0 = 0;
                float thermal_delay1 = 0;
		float thermal_delay2 = 0;

                float BCT0 = free_slots_neighbor[directions[0]].read();
                float BCT1 = free_slots_neighbor[directions[1]].read();
		float BCT2 = free_slots_neighbor[directions[2]].read();

                thermal_delay0 = buf_neighbor[directions[0]][directions[0]].read() + BCT0;
                thermal_delay1 = buf_neighbor[directions[1]][directions[1]].read() + BCT1;
		thermal_delay2 = buf_neighbor[directions[2]][directions[2]].read() + BCT2;
/*
		if(thermal_delay2 < thermal_delay1 && thermal_delay2 < thermal_delay0)
                        return directions[2];
                else if(thermal_delay1 < thermal_delay0 && thermal_delay1 < thermal_delay2)
                        return directions[1];
                else
                        return directions[0];
*/
		//WF3D offer zero or three path
		if(directions[0]==DIRECTION_NORTH && directions[1]==DIRECTION_EAST && directions[2]==DIRECTION_DOWN){
			thermal_delay0 += buf_neighbor[DIRECTION_EAST][directions[0]].read()/5 + buf_neighbor[DIRECTION_WEST][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/5 + buf_neighbor[DIRECTION_EAST][directions[0]].read()/5 + buf_neighbor[DIRECTION_SOUTH][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/5 + buf_neighbor[DIRECTION_DOWN][directions[0]].read()/6 + buf_neighbor[DIRECTION_EAST][directions[0]].read()/6 + buf_neighbor[DIRECTION_WEST][directions[0]].read()/6 + buf_neighbor[DIRECTION_SOUTH][directions[0]].read()/6 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read())*17/30;
			thermal_delay1 += buf_neighbor[DIRECTION_DOWN][directions[1]].read()/3 + buf_neighbor[DIRECTION_WEST][directions[1]].read()/3 + buf_neighbor[DIRECTION_NORTH][directions[1]].read()/3 + buf_neighbor[DIRECTION_DOWN][directions[1]].read()/3 + buf_neighbor[DIRECTION_SOUTH][directions[1]].read()/3 + buf_neighbor[DIRECTION_NORTH][directions[1]].read()/3 + buf_neighbor[DIRECTION_DOWN][directions[1]].read()/4 + buf_neighbor[DIRECTION_NORTH][directions[1]].read()/4 + buf_neighbor[DIRECTION_SOUTH][directions[1]].read()/4 + buf_neighbor[DIRECTION_WEST][directions[1]].read()/4 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read())*17/30;
			thermal_delay2 += (buf_neighbor[DIRECTION_EAST][directions[2]].read()+buf_neighbor[DIRECTION_NORTH][directions[2]].read())*17/30 + (buf_neighbor[DIRECTION_WEST][directions[2]].read()+buf_neighbor[DIRECTION_SOUTH][directions[2]].read())*11/30 + (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[2]].read()+buf_neighbor[DIRECTION_LOCAL][directions[2]].read())*17/30;
		}
		else if(directions[0]==DIRECTION_SOUTH && directions[1]==DIRECTION_EAST && directions[2]==DIRECTION_DOWN){
			thermal_delay0 += (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_LOCAL][directions[0]].read()+buf_neighbor[DIRECTION_EAST][directions[0]].read()+buf_neighbor[DIRECTION_DOWN][directions[0]].read())*17/30 + (buf_neighbor[DIRECTION_NORTH][directions[0]].read()+buf_neighbor[DIRECTION_WEST][directions[0]].read())*11/30;
			thermal_delay1 += (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_LOCAL][directions[1]].read()+buf_neighbor[DIRECTION_DOWN][directions[1]].read()+buf_neighbor[DIRECTION_SOUTH][directions[1]].read())*17/30+ (buf_neighbor[DIRECTION_NORTH][directions[1]].read()+buf_neighbor[DIRECTION_WEST][directions[1]].read())*11/30;
			thermal_delay2 += (buf_neighbor[DIRECTION_SEMI_LOCAL][directions[2]].read()+buf_neighbor[DIRECTION_LOCAL][directions[2]].read()+buf_neighbor[DIRECTION_EAST][directions[2]].read()+buf_neighbor[DIRECTION_SOUTH][directions[2]].read())*17/30+ (buf_neighbor[DIRECTION_NORTH][directions[2]].read()+buf_neighbor[DIRECTION_WEST][directions[2]].read())*11/30;
		}
		//PTMAR
	
                if(thermal_delay2 < thermal_delay1 && thermal_delay2 < thermal_delay0)
                        return directions[2];
                else if(thermal_delay1 < thermal_delay0 && thermal_delay1 < thermal_delay2)
                        return directions[1];
		else
			return directions[0];
                
        }
        else
                return directions[rand() % directions.size()];


	
}

int NoximRouter::selectionRandom(const vector < int >&directions)
{
    return directions[rand() % directions.size()];
}
vector<int> NoximRouter::DW_layerSelFunction    (const int select_routing, const NoximCoord& current, const NoximCoord& destination, const NoximCoord& source, int dw_layer){
	switch ( NoximGlobalParams::dw_layer_sel ){
	case DW_BL      :return routingTLAR_DW         (current, source, destination          );break;
	case DW_ODWL    :return routingTLAR_DW_ODWL    (current, source, destination, dw_layer);break;
	case DW_ADWL    :return routingTLAR_DW_ADWL    (current, source, destination          );break;
	case DW_IPD     :return routingTLAR_DW_IPD     (current, source, destination          );break;
	case DW_VBDR    :return routingTLAR_DW_VBDR    (current, source, destination          );break;
	case DW_ODWL_IPD:return routingTLAR_DW_ODWL_IPD(current, source, destination, dw_layer);break;
	default         :return routingTLAR_DW         (current, source, destination          );break;
	}
}
int NoximRouter::selectionFunction(const vector < int >&directions,
				   const NoximRouteData & route_data)
{
    // not so elegant but fast escape ;)
    if (directions.size() == 1)
	return directions[0];

    switch (NoximGlobalParams::selection_strategy) {
    case SEL_RANDOM      :return selectionRandom     (directions);
    case SEL_BUFFER_LEVEL:return selectionBufferLevel(directions);
    case SEL_NOP         :return selectionNoP        (directions, route_data);
	case SEL_RCA         :return selectionRCA2D      (directions, route_data);
	case SEL_PROPOSED    :return selectionProposed   (directions, route_data);
	case SEL_THERMAL     :return selectionThermal    (directions, route_data);	
	default              :assert(false);
    }
    return 0;
}

vector < int >NoximRouter::routingXYZ(const NoximCoord & current,const NoximCoord & destination){
    vector < int >directions;
	     if (destination.x > current.x)	directions.push_back(DIRECTION_EAST );
    else if (destination.x < current.x)	directions.push_back(DIRECTION_WEST );
    else if (destination.y > current.y)	directions.push_back(DIRECTION_SOUTH);
    else if (destination.y < current.y)	directions.push_back(DIRECTION_NORTH);
	else if (destination.z > current.z)	directions.push_back(DIRECTION_DOWN );
    else if (destination.z < current.z)	directions.push_back(DIRECTION_UP   );
    return directions;
}
vector<int> NoximRouter::routingZXY(const NoximCoord& current, const NoximCoord& destination){
  vector<int> directions;
       if (destination.z > current.z) directions.push_back(DIRECTION_DOWN );
  else if (destination.z < current.z) directions.push_back(DIRECTION_UP   );
  else if (destination.x > current.x) directions.push_back(DIRECTION_EAST );
  else if (destination.x < current.x) directions.push_back(DIRECTION_WEST );
  else if (destination.y > current.y) directions.push_back(DIRECTION_SOUTH);
  else if (destination.y < current.y) directions.push_back(DIRECTION_NORTH);
  return directions;
}

vector<int> NoximRouter::routingDownward(const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination){

	int down_level = NoximGlobalParams::down_level;
	vector<int> directions;
	int layer;	//means which layer that packet should be transmitted
	
	if( (source.z + down_level)> NoximGlobalParams::mesh_dim_z-1)
		layer = NoximGlobalParams::mesh_dim_z-1;
	else 
		layer = source.z + down_level;

	if(current.z < layer && (current.x != destination.x || current.y != destination.y) ){
			if(current.z == destination.z && ( (current.x-destination.x== 1 && current.y==destination.y)	//while the dest. is one hop to source, don't downward and transmit directly
																			 ||(current.x-destination.x==-1 && current.y==destination.y) 
																			 ||(current.y-destination.y== 1 && current.x==destination.x) 
																			 ||(current.y-destination.y==-1 && current.x==destination.x) ))
			{
				directions = routingXYZ(current, destination);	 
			}
			else {
				directions.push_back(DIRECTION_DOWN);
			}
		}
		else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) )	//forward by xyz routing
		{
			 directions = routingXYZ(current, destination);	 
		}
		else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z)	//same (x,y), forward to up
		{	
			directions.push_back(DIRECTION_UP);
		}	
		else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z)  //same (x,y), forward to down
		{	
			directions.push_back(DIRECTION_DOWN);
		}	
		else 
		{ 
			cout<<"ERROR! Out of condition!?!?"<<endl;
			cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
			cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
			cout<<"layer: "<<layer<<endl;
			exit(1);			
		}

  return directions;
}

vector<int> NoximRouter::routingWF_Downward(const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination){

	int down_level = NoximGlobalParams::down_level;
	vector<int> directions;
	int layer;	//means which layer that packet should be transmitted
	
	if( (source.z + down_level)> NoximGlobalParams::mesh_dim_z-1)
		layer = NoximGlobalParams::mesh_dim_z-1;
	else 
		layer = source.z + down_level;

	if(current.z < layer && (current.x != destination.x || current.y != destination.y) ){
			if(current.z == destination.z && ( (current.x-destination.x== 1 && current.y==destination.y)	//while the dest. is one hop to source, don't downward and transmit directly
																			 ||(current.x-destination.x==-1 && current.y==destination.y) 
																			 ||(current.y-destination.y== 1 && current.x==destination.x) 
																			 ||(current.y-destination.y==-1 && current.x==destination.x) ))
			{
				directions = routingWestFirst(current, destination);	 
			}
			else {
				directions.push_back(DIRECTION_DOWN);
			}
		}
		else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) )	//forward by xyz routing
		{
			 directions = routingWestFirst(current, destination);	 
		}
		else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z)	//same (x,y), forward to up
		{	
			directions.push_back(DIRECTION_UP);
		}	
		else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z)  //same (x,y), forward to down
		{	
			directions.push_back(DIRECTION_DOWN);
		}	
		else 
		{ 
			cout<<"ERROR! Out of condition!?!?"<<endl;
			cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
			cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
			cout<<"layer: "<<layer<<endl;
			exit(1);			
		}

  return directions;
}

vector < int >NoximRouter::routingWestFirst(const NoximCoord & current,
					    const NoximCoord & destination)
{

    vector < int >directions;
    if (destination.x <= current.x || destination.z < current.z || (destination.y == current.y && destination.z == current.z))
	return routingXYZ(current, destination);
  
    if (destination.y < current.y && destination.z == current.z) {
        directions.push_back(DIRECTION_NORTH);
        directions.push_back(DIRECTION_EAST);
    } else if(destination.y > current.y && destination.z == current.z){
        directions.push_back(DIRECTION_SOUTH);
        directions.push_back(DIRECTION_EAST);
    } else
    if (destination.y < current.y && destination.z > current.z) {
	directions.push_back(DIRECTION_NORTH);
	directions.push_back(DIRECTION_EAST);
	directions.push_back(DIRECTION_DOWN);
    } else if(destination.y > current.y && destination.z > current.z){
	directions.push_back(DIRECTION_SOUTH);
	directions.push_back(DIRECTION_EAST);
	directions.push_back(DIRECTION_DOWN);
    } else{
	//directions.clear();
	return routingXYZ(current, destination);
	}
    //cout<<"dir"<<directions.size()<<endl;
    return directions;

/*
    vector < int >directions;

    if (destination.x <= current.x ||  destination.y == current.y)
        return routingXYZ(current, destination);

    if (destination.y < current.y) {
        directions.push_back(DIRECTION_NORTH);
        directions.push_back(DIRECTION_EAST);
    } else {
        directions.push_back(DIRECTION_SOUTH);
        directions.push_back(DIRECTION_EAST);
    }

    return directions;*/
}

vector < int >NoximRouter::routingNorthLast(const NoximCoord & current,
					    const NoximCoord & destination)
{
    vector < int >directions;

    if (destination.x == current.x || destination.y <= current.y)
	return routingXYZ(current, destination);

    if (destination.x < current.x) {
	directions.push_back(DIRECTION_SOUTH);
	directions.push_back(DIRECTION_WEST);
    } else {
	directions.push_back(DIRECTION_SOUTH);
	directions.push_back(DIRECTION_EAST);
    }

    return directions;
}

vector < int >NoximRouter::routingNegativeFirst(const NoximCoord & current,
						const NoximCoord &
						destination)
{
    vector < int >directions;

    if ((destination.x <= current.x && destination.y <= current.y) ||
	(destination.x >= current.x && destination.y >= current.y))
	return routingXYZ(current, destination);

    if (destination.x > current.x && destination.y < current.y) {
	directions.push_back(DIRECTION_NORTH);
	directions.push_back(DIRECTION_EAST);
    } else {
	directions.push_back(DIRECTION_SOUTH);
	directions.push_back(DIRECTION_WEST);
    }

    return directions;
}

vector < int >NoximRouter::routingOddEven(const NoximCoord & current,
					  const NoximCoord & source,
					  const NoximCoord & destination)
{
    vector < int >directions;

    int c0 = current.x;
    int c1 = current.y;
    int s0 = source.x;
    //  int s1 = source.y;
    int d0 = destination.x;
    int d1 = destination.y;
    int e0, e1;

    e0 = d0 - c0;
    e1 = -(d1 - c1);

    if (e0 == 0) {
	if (e1 > 0)
	    directions.push_back(DIRECTION_NORTH);
	else
	    directions.push_back(DIRECTION_SOUTH);
    } else {
	if (e0 > 0) {
	    if (e1 == 0)
		directions.push_back(DIRECTION_EAST);
	    else {
		if ((c0 % 2 == 1) || (c0 == s0)) {
		    if (e1 > 0)
			directions.push_back(DIRECTION_NORTH);
		    else
			directions.push_back(DIRECTION_SOUTH);
		}
		if ((d0 % 2 == 1) || (e0 != 1))
		    directions.push_back(DIRECTION_EAST);
	    }
	} else {
	    directions.push_back(DIRECTION_WEST);
	    if (c0 % 2 == 0) {
		if (e1 > 0)
		    directions.push_back(DIRECTION_NORTH);
		if (e1 < 0)
		    directions.push_back(DIRECTION_SOUTH);
	    }
	}
    }

    if (!(directions.size() > 0 && directions.size() <= 2)) {
	cout << "\n PICCININI, CECCONI & ... :";	// STAMPACCHIA
	cout << source << endl;
	cout << destination << endl;
	cout << current << endl;
    }
    assert(directions.size() > 0 && directions.size() <= 2);
    return directions;
}
/*=============================Odd Even-3D==========================*/
vector<int> NoximRouter::routingOddEven_for_3D(const NoximCoord& current, 
				    const NoximCoord& source, const NoximCoord& destination)
{
	vector<int> directions;
	
	int c0 = current.y;
	int c1 = current.z;
	int s0 = source.y;
	//  int s1 = source.y;
	int d0 = destination.y;
	int d1 = destination.z;
	int e0, e1;
	
	e0 = d0 - c0;
	e1 = d1 - c1;

	if (e0 == 0){
		if (e1 > 0)
			directions.push_back(DIRECTION_DOWN);
		else if (e1 < 0)
			directions.push_back(DIRECTION_UP);
    }
	else{
		if (e0 > 0){
			if (e1 == 0)
				directions.push_back(DIRECTION_SOUTH);
			else{
				if ( (c0 % 2 == 1) || (c0 == s0) ){ //Odd
					if (e1 > 0)
						directions.push_back(DIRECTION_DOWN);
					else if (e1 < 0)
						directions.push_back(DIRECTION_UP);
				}
				if ( (d0 % 2 == 1) || (e0 != 1) )
					directions.push_back(DIRECTION_SOUTH);
			}
		}
		else{
			directions.push_back(DIRECTION_NORTH);
			if (c0 % 2 == 0){
				if (e1 > 0)
					directions.push_back(DIRECTION_DOWN);
				if (e1 < 0) 
					directions.push_back(DIRECTION_UP);
			}
		}
    }
	return directions;
}
vector<int> NoximRouter::routingOddEven_3D (const NoximCoord& current, 
				    const NoximCoord& source, const NoximCoord& destination)
{
  vector<int> directions;

  int c0 = current.x;
  int c1 = current.y;
  int c2 = current.z;
  int s0 = source.x;
  int s1 = source.y;
  int s2 = source.z;
  int d0 = destination.x;
  int d1 = destination.y;
  int d2 = destination.z; //z = 0, which is far from heat sink
  int e0, e1, e2;

  e0 = d0 - c0;
  e1 = -(d1 - c1); //positive: North, negative: South
  e2 = d2 - c2; //positive: Down, negative: Up

  if (e0 == 0){
	directions = routingOddEven_for_3D(current, source, destination);
  }
  else{
	if (e0 < 0){ //x-
		if ( (c0 % 2 == 0) ){
			directions = routingOddEven_for_3D(current, source, destination);
		}
		directions.push_back(DIRECTION_WEST);
	}
	else{ //e0 > 0, x+
		if( (e1 == 0) && (e2 == 0) )
			directions.push_back(DIRECTION_EAST);	
		else{
			if ( (d0 % 2 == 1) || (e0 != 1) ){
				directions.push_back(DIRECTION_EAST);
			}
			if ( (c0 % 2 == 1) || (c0 == s0) ){
				directions = routingOddEven_for_3D(current, source, destination);
			}
		}
	}
  }
  if (!(directions.size() > 0 && directions.size() <= 3)){
      cout << "\n STAMPACCHIO :";
      cout << source << endl;
      cout << destination << endl;
      cout << current << endl;
  }
  //assert(directions.size() > 0 && directions.size() <= 3);
  
  return directions;
}
//=============================Odd Even + Downward==========================
//舊版，吃全域的Downward tag

vector<int> NoximRouter::routingOddEven_Downward (const NoximCoord& current, 
				    const NoximCoord& source, const NoximCoord& destination, const NoximRouteData& route_data)
{
	vector<int> directions;
	int layer;	//表示此packet應該在哪個layer做XY傳遞
	/*int down_level = NoximGlobalParams::down_level;
	if( (source.z + down_level)> NoximGlobalParams::mesh_dim_z-1)
		layer = NoximGlobalParams::mesh_dim_z-1;
	else 
		layer = source.z + down_level;*/
	layer = NoximGlobalParams::mesh_dim_z-1;
	if(current.z < layer && (current.x != destination.x || current.y != destination.y) )  
	{

		if(current.z == destination.z && ( (current.x-destination.x== 1 && current.y==destination.y)	//X or Y方向上只相距一格時就直接過去,不DW
										||(current.x-destination.x==-1 && current.y==destination.y) 
										||(current.y-destination.y== 1 && current.x==destination.x) 
										||(current.y-destination.y==-1 && current.x==destination.x) ))
		{
			directions = routingOddEven(current, source, destination);	 
		}
		else 
		{
					directions.push_back(DIRECTION_DOWN);
		}
	}
	else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) )	//在底部以xy routing傳
	{
			directions = routingOddEven(current, source, destination);
	}
	else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z)	//xy相同, z方向往上傳
	{	
		directions.push_back(DIRECTION_UP);
	}	
	else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z)  //xy相同, z方向往下傳
	{	
		directions.push_back(DIRECTION_DOWN);
	}	
	else 
	{ 
		cout<<"ERROR! Out of condition!?!?"<<endl;
		cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
		cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
		cout<<"layer: "<<layer<<endl;
		exit(1);			
	}
  assert(directions.size() > 0 && directions.size() <= 3);
  return directions;
}
vector<int> NoximRouter::routingOddEven_Z (const NoximCoord& current, 
				    const NoximCoord& source, const NoximCoord& destination)
{
	vector<int> directions;

	if(current.x == destination.x && current.y == destination.y){
		if(current.z < destination.z)
			directions.push_back(DIRECTION_DOWN);
		else
			directions.push_back(DIRECTION_UP);
	}
	else
		directions = routingOddEven(current, source, destination);	
	
	if (!(directions.size() > 0 && directions.size() <= 3)){
      cout << "\n STAMPACCHIO :";
      cout << source << endl;
      cout << destination << endl;
      cout << current << endl;
	}
	assert(directions.size() > 0 && directions.size() <= 3);
	return directions;	
}

vector < int >NoximRouter::routingDyAD(const NoximCoord & current,
				       const NoximCoord & source,
				       const NoximCoord & destination)
{
    vector < int >directions;

    directions = routingOddEven(current, source, destination);

    if (!inCongestion())
	directions.resize(1);

    return directions;
}

vector<int> NoximRouter::routingFullyAdaptive   (const NoximCoord& current                          , const NoximCoord& destination){
    vector < int >directions;
/*
    if        (destination.x == current.x || destination.y == current.y)
		return routingXYZ(current, destination);
*/
if(destination.z > current.z){
    if        (destination.x == current.x || destination.y == current.y)
                return routingXYZ(current, destination);

    if        (destination.x > current.x && destination.y < current.y) {
		directions.push_back(DIRECTION_NORTH);
		directions.push_back(DIRECTION_EAST);
		directions.push_back(DIRECTION_DOWN);
    } else if (destination.x > current.x && destination.y > current.y) {
		directions.push_back(DIRECTION_SOUTH);
		directions.push_back(DIRECTION_EAST);
		directions.push_back(DIRECTION_DOWN);
    } else if (destination.x < current.x && destination.y > current.y) {
		directions.push_back(DIRECTION_SOUTH);
		directions.push_back(DIRECTION_WEST);
		directions.push_back(DIRECTION_DOWN);
    } else {
		directions.push_back(DIRECTION_NORTH);
		directions.push_back(DIRECTION_WEST);
		directions.push_back(DIRECTION_DOWN);
    }
}
else if(destination.z < current.z){
   if        (destination.x == current.x || destination.y == current.y)
                return routingXYZ(current, destination);

   if        (destination.x > current.x && destination.y < current.y) {
                directions.push_back(DIRECTION_NORTH);
                directions.push_back(DIRECTION_EAST);
                directions.push_back(DIRECTION_UP);
    } else if (destination.x > current.x && destination.y > current.y) {
                directions.push_back(DIRECTION_SOUTH);
                directions.push_back(DIRECTION_EAST);
                directions.push_back(DIRECTION_UP);
    } else if (destination.x < current.x && destination.y > current.y) {
                directions.push_back(DIRECTION_SOUTH);
                directions.push_back(DIRECTION_WEST);
                directions.push_back(DIRECTION_UP);
    } else {
                directions.push_back(DIRECTION_NORTH);
                directions.push_back(DIRECTION_WEST);
                directions.push_back(DIRECTION_UP);
    }
}
else{
   if        (destination.x == current.x || destination.y == current.y)
                return routingXYZ(current, destination);

   if        (destination.x > current.x && destination.y < current.y) {
                directions.push_back(DIRECTION_NORTH);
                directions.push_back(DIRECTION_EAST);
//                directions.push_back(DIRECTION_UP);
    } else if (destination.x > current.x && destination.y > current.y) {
                directions.push_back(DIRECTION_SOUTH);
                directions.push_back(DIRECTION_EAST);
  //              directions.push_back(DIRECTION_UP);
    } else if (destination.x < current.x && destination.y > current.y) {
                directions.push_back(DIRECTION_SOUTH);
                directions.push_back(DIRECTION_WEST);
    //            directions.push_back(DIRECTION_UP);
    } else {
                directions.push_back(DIRECTION_NORTH);
                directions.push_back(DIRECTION_WEST);
      //          directions.push_back(DIRECTION_UP);
    }


}


    return directions;
}
vector<int> NoximRouter::routingTableBased      (const NoximCoord& current, const int dir_in        , const NoximCoord& destination){
    NoximAdmissibleOutputs ao =
	routing_table.getAdmissibleOutputs(dir_in, coord2Id(destination));

    if (ao.size() == 0) {
	cout << "dir: " << dir_in << ", (" << current.x << "," << current.
	    y << ") --> " << "(" << destination.x << "," << destination.
	    y << ")" << endl << coord2Id(current) << "->" <<
	    coord2Id(destination) << endl;
    }
    assert(ao.size() > 0);
    //-----
    /*
       vector<int> aov = admissibleOutputsSet2Vector(ao);
       cout << "dir: " << dir_in << ", (" << current.x << "," << current.y << ") --> "
       << "(" << destination.x << "," << destination.y << "), outputs: ";
       for (int i=0; i<aov.size(); i++)
       cout << aov[i] << ", ";
       cout << endl;
     */
    //-----
    return admissibleOutputsSet2Vector(ao);
}
vector<int> NoximRouter::routingDLADR           (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination,const int select_routing, int dw_layer){
	switch ( select_routing ){
	case ROUTING_XYZ                 :return routingXYZ         (                 current, destination                   );break;
	case ROUTING_WEST_FIRST          :return routingWestFirst   (                 current, destination                   );break;
	//case ROUTING_FULLY_ADAPTIVE      :return routingFullyAdaptive(                 current, destination                   );break;
	case ROUTING_ODD_EVEN_3D         :return routingOddEven_3D      (current , source, destination);break;
	case ROUTING_DOWNWARD_CROSS_LAYER:return DW_layerSelFunction( select_routing, current, destination, source, dw_layer );break;
	default                          :cout<<getCurrentCycleNum()<<":Wrong with Cross-Layer!"<<endl;
	                                  cout<<"Current    :"<<current<<endl;
									  cout<<"Source     :"<<source <<endl;
									  cout<<"Destination:"<<destination<<endl;
									  cout<<"Select routing:"<<select_routing<<endl;assert(false)                                 ;break;
	}
}
vector<int> NoximRouter::routingDLAR            (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination,const int select_routing, int dw_layer){
	switch ( select_routing ){
	case ROUTING_WEST_FIRST          :return routingOddEven_Z   (                 current, source, destination           );break;
	case ROUTING_XYZ                 :return DW_layerSelFunction( select_routing, current, destination, source, dw_layer );break;
	case ROUTING_DOWNWARD_CROSS_LAYER:return DW_layerSelFunction( select_routing, current, destination, source, dw_layer );break;
	default                          :cout<<"Wrong with Cross-Layer!"<<select_routing<<endl;assert(false)                 ;break;
	}
}
vector<int> NoximRouter::routingDLDR            (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination,const int select_routing, int dw_layer){
	switch ( select_routing ){
	case ROUTING_XYZ                 :return routingXYZ         (                 current, destination                   );break;
	case ROUTING_WEST_FIRST          :return routingXYZ         (                 current, destination                   );break;
	case ROUTING_DOWNWARD_CROSS_LAYER:return DW_layerSelFunction( select_routing, current, destination, source, dw_layer );break;
	default                          :cout<<"Wrong with Cross-Layer!"<<endl;assert(false)                                 ;break;
	}
}
vector<int> NoximRouter::routingTLAR_DW         (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination){
	vector<int> directions;
	int layer = NoximGlobalParams::mesh_dim_z - 1 ;	//表示此packet應該在哪個layer做XY傳遞
	   if(current.z < layer && (current.x != destination.x || current.y != destination.y) )  
		{
            directions.push_back(DIRECTION_DOWN);
		}
		else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) )	//在底部以xy routing傳
		{
			switch (NoximGlobalParams::routing_algorithm){
			case ROUTING_DLAR : directions = routingOddEven_Z(current, source, destination);break;
			case ROUTING_DLADR: directions = routingWestFirst(current,         destination);break;
			default           :	directions = routingXYZ      (current,         destination);break;
			}
		}
		else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z)
		{	
			directions.push_back(DIRECTION_UP);
		}	
		else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z)  
		{	
			directions.push_back(DIRECTION_DOWN);
		}	
		else 
		{ 
			cout<<"ERROR! Out of condition!?!?"<<endl;
			cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
			cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
			cout<<"layer: "<<layer<<endl;
			exit(1);			
		}
  return directions;
}
vector<int> NoximRouter::routingTLAR_DW_VBDR    (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination){
	vector<int> directions;
	int max = 0;
	int layer = 3;
	int free_slot[4];
	NoximNoP_data nop_tmp[4];
	for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ ){
		nop_tmp[i] = vertical_free_slot_in[i].read();
		free_slot[i] = 0 ;
		}
	if ( destination.x > current.x ){
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_EAST].free_slots;
	}
	else{
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_WEST].free_slots;
	}
	if ( destination.y > current.y ){
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_SOUTH].free_slots;
	}
	else{
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_NORTH].free_slots;
	}
	for( int i = 2 ; i < NoximGlobalParams::mesh_dim_z ; i++ ){
		if( free_slot[i] > max ){
			layer = i;
			max = free_slot[i];
		}		
	}
	
	if(current.z < layer && (current.x != destination.x || current.y != destination.y) )  
		{
			//Increasing Path Diversity
			if( !on_off_neighbor[0] && !on_off_neighbor[1] && !on_off_neighbor[2] && !on_off_neighbor[3] )
				directions = routingXYZ(current, destination);
			directions.push_back(DIRECTION_DOWN);
		}
		else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) )	//在底部以xy routing傳
		{
			switch (NoximGlobalParams::routing_algorithm){
			case ROUTING_DLAR : directions = routingOddEven_Z(current, source, destination);break;
			case ROUTING_DLADR: directions = routingWestFirst(current,         destination);break;
			default           :	directions = routingXYZ      (current,         destination);break;
			}
		}
		else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z)	//xy相同, z方向往上傳
		{	
			directions.push_back(DIRECTION_UP);
		}	
		else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z)  //xy相同, z方向往下傳
		{	
			directions.push_back(DIRECTION_DOWN);
		}	
		else 
		{ 
			cout<<"ERROR! Out of condition!?!?"<<endl;
			cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
			cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
			cout<<"layer: "<<layer<<endl;
			exit(1);			
		}
	return directions;
}
vector<int> NoximRouter::routingTLAR_DW_ODWL_IPD(const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination, int dw_layer){
	vector<int> directions;
	int layer = dw_layer;
	if(current.z < layer && (current.x != destination.x || current.y != destination.y) ){
		if( NoximGlobalParams::routing_algorithm == ROUTING_DLAR )
			directions = routingOddEven(current, source, destination);
		else if ( NoximGlobalParams::routing_algorithm == ROUTING_DLADR )
			directions = routingWestFirst(current, destination);
		else
			directions = routingXYZ(current, destination);
			
		for(int i = directions.size() - 1  ; i >= 0 ; i--){
			if ( directions[i] < 4 ) //for lateral direction
				if ( on_off_neighbor[ directions[i] ].read() == 1 ){//if the direction is throttled
					directions.erase( directions.begin() + i);//then erase that way	
			}
		}
	
		if(current.z == destination.z && ( (current.x-destination.x== 1 && current.y==destination.y)	//while the dest. is one hop to source, don't downward and transmit directly
										 ||(current.x-destination.x==-1 && current.y==destination.y) 
										 ||(current.y-destination.y== 1 && current.x==destination.x) 
										 ||(current.y-destination.y==-1 && current.x==destination.x) ))
			directions = routingOddEven(current, source, destination);
		else 
			directions.push_back(DIRECTION_DOWN);
	}
	else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) ){	//在底部以xy routing傳
		switch (NoximGlobalParams::routing_algorithm){
			case ROUTING_DLAR : directions = routingOddEven_Z(current, source, destination);break;
			case ROUTING_DLADR: directions = routingWestFirst(current,         destination);break;
			default           :	directions = routingXYZ      (current,         destination);break;
		}
	}
	else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z){	//xy相同, z方向往上傳
		directions.push_back(DIRECTION_UP);
	}	
	else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z){  //xy相同, z方向往下傳
		directions.push_back(DIRECTION_DOWN);
	}	
	else{ 
		cout<<"ERROR! Out of condition!?!?"<<endl;
		cout<<"current:     "<<current    <<endl;
		cout<<"destination: "<<destination<<endl;
		cout<<"layer:       "<<layer      <<endl;
		exit(1);			
	}
	if( directions.size() == 0 ){
		cout<<"current:     "<<current    <<endl;
		cout<<"source:      "<<source     <<endl;
		cout<<"destination: "<<destination<<endl;
		assert(false);
		}
	return directions;
}
vector<int> NoximRouter::routingTLAR_DW_ADWL    (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination){
	vector<int> directions;
	int max = 0;
	int layer = 3;              //表示此packet應該在哪個layer做XY傳遞
	int free_slot[4];
	NoximNoP_data nop_tmp[4];
	for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ ){
		nop_tmp[i] = vertical_free_slot_in[i].read();
		free_slot[i] = 0 ;
		}
	if ( destination.x > current.x ){
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_EAST].free_slots;
	}
	else{
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_WEST].free_slots;
	}
	if ( destination.y > current.y ){
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_SOUTH].free_slots;
	}
	else{
		for( int i = 0 ; i < NoximGlobalParams::mesh_dim_z ; i++ )
			free_slot[i] = nop_tmp[i].channel_status_neighbor[DIRECTION_NORTH].free_slots;
	}
	for( int i = 3 ; i < NoximGlobalParams::mesh_dim_z ; i++ ){
		if( free_slot[i] > max ){
			layer = i;
			max = free_slot[i];
		}		
	}	
	
	if(current.z < layer && (current.x != destination.x || current.y != destination.y) )  
		{
			directions.push_back(DIRECTION_DOWN);
		}
		else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) )	//在底部以xy routing傳
		{
			switch (NoximGlobalParams::routing_algorithm){
			case ROUTING_DLAR : directions = routingOddEven_Z(current, source, destination);break;
			case ROUTING_DLADR: directions = routingWestFirst(current,         destination);break;
			default           :	directions = routingXYZ      (current,         destination);break;
			}
		}
		else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z)	//xy相同, z方向往上傳
		{	
			directions.push_back(DIRECTION_UP);
		}	
		else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z)  //xy相同, z方向往下傳
		{	
			directions.push_back(DIRECTION_DOWN);
		}	
		else 
		{ 
			cout<<"ERROR! Out of condition!?!?"<<endl;
			cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
			cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
			cout<<"layer: "<<layer<<endl;
			exit(1);			
		}
	return directions;
}
vector<int> NoximRouter::routingTLAR_DW_ODWL    (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination, int dw_layer){
	vector<int> directions;
	int layer = dw_layer;
	
	if(current.z < layer && (current.x != destination.x || current.y != destination.y) )  
		{
			//Increasing Path Diversity
			if( !on_off_neighbor[0] && !on_off_neighbor[1] && !on_off_neighbor[2] && !on_off_neighbor[3] && (current.z == source.z) ){
				if ( NoximGlobalParams::routing_algorithm == ROUTING_DLAR ){
					directions = routingOddEven_Z(current, source, destination);
					bool adaptive = true;
					for(int i = 0 ; i < directions.size() ; i++){
						NoximCoord sour =source;
						NoximCoord fake =id2Coord( getNeighborId(local_id, directions[i]) ); 
						if( !Adaptive_ok( sour, fake ) )
							adaptive = false;
					}
					if(!adaptive)directions.clear();
				}
				else
					directions = routingXYZ(current, destination);
			}
			if(current.z == destination.z && ( (current.x-destination.x== 1 && current.y==destination.y)	//while the dest. is one hop to source, don't downward and transmit directly
										 ||(current.x-destination.x==-1 && current.y==destination.y) 
										 ||(current.y-destination.y== 1 && current.x==destination.x) 
										 ||(current.y-destination.y==-1 && current.x==destination.x) ))
				directions = routingOddEven_Z(current, source, destination);
			else
				directions.push_back(DIRECTION_DOWN);
		}
		else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) )	//在底部以xy routing傳
		{
			switch (NoximGlobalParams::routing_algorithm){
			case ROUTING_DLAR : directions = routingOddEven_Z(current, source, destination);break;
			case ROUTING_DLADR: directions = routingWestFirst(current,         destination);break;
			default           :	directions = routingXYZ      (current,         destination);break;
			}
		}
		else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z)	//xy相同, z方向往上傳
		{	
			directions.push_back(DIRECTION_UP);
		}	
		else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z)  //xy相同, z方向往下傳
		{	
			directions.push_back(DIRECTION_DOWN);
		}	
		else 
		{ 
			cout<<"ERROR! Out of condition!?!?"<<endl;
			cout<<"Router ID:"<<local_id<<endl;
			cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
			cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
			cout<<"layer: "<<layer<<endl;
			assert(0);
			exit(1);			
		}
	assert(directions.size() > 0 );
	assert(directions.size() <= 3);
	return directions;
}	
vector<int> NoximRouter::routingTLAR_DW_IPD     (const NoximCoord& current, const NoximCoord& source, const NoximCoord& destination){
	vector<int> directions;
	int layer = NoximGlobalParams::mesh_dim_z - 1;	
	
	if(current.z < layer && (current.x != destination.x || current.y != destination.y) ) {
		//Increasing Path Diversity
		if( !on_off_neighbor[0] && !on_off_neighbor[1] && !on_off_neighbor[2] && !on_off_neighbor[3] && (current.z == source.z)){
			if      ( NoximGlobalParams::routing_algorithm == ROUTING_DLAR ){
				directions = routingOddEven_Z(current, source, destination);
				bool adaptive = true;
				for(int i = 0 ; i < directions.size() ; i++){
					NoximCoord sour =source;
					NoximCoord fake =id2Coord( getNeighborId(local_id, directions[i]) ); 
					if( !Adaptive_ok( sour, fake ) )
						adaptive = false;
				}
				if(!adaptive)directions.clear();
				}
			else
				directions = routingXYZ(current, destination);
		}
		directions.push_back(DIRECTION_DOWN);
	}
	else if(current.z >=layer && (current.x != destination.x || current.y != destination.y) ){
		switch (NoximGlobalParams::routing_algorithm){
		case ROUTING_DLAR : directions = routingOddEven_Z(current, source, destination);break;
		case ROUTING_DLADR: directions = routingWestFirst(current,         destination);break;
		default           :	directions = routingXYZ      (current,         destination);break;
		}
	}
	else if((current.x == destination.x && current.y == destination.y) && current.z > destination.z){
		directions.push_back(DIRECTION_UP);
	}	
	else if((current.x == destination.x && current.y == destination.y) && current.z < destination.z){
		directions.push_back(DIRECTION_DOWN);
	}	
	else{ 
		cout<<"ERROR! Out of condition!?!?"<<endl;
		cout<<"current: ("<<current.x<<","<<current.y<<","<<current.z<<")"<<endl;
		cout<<"destination: ("<<destination.x<<","<<destination.y<<","<<destination.z<<")"<<endl;
		cout<<"layer: "<<layer<<endl;
		exit(1);			
	}
	return directions;
}	

void NoximRouter::configure(const int _id,
			    const double _warm_up_time,
			    const unsigned int _max_buffer_size,
			    NoximGlobalRoutingTable & grt)
{
    local_id = _id;
    stats.configure(_id, _warm_up_time);

    start_from_port = DIRECTION_LOCAL;

    if (grt.isValid())
	routing_table.configure(grt, _id);

	//if( !NoximGlobalParams::buffer_alloc )
	for (int i = 0; i < DIRECTIONS + 1; i++)
		buffer[i].SetMaxBufferSize(_max_buffer_size);
	/*else{
		int z = id2Coord(_id).z;
		switch(z){
			case 0:
				for (int i = 0; i < 4; i++)
					buffer[i].SetMaxBufferSize(BUFFER_L_LAYER_0);
				buffer[DIRECTION_DOWN].SetMaxBufferSize(BUFFER_D_LAYER_0);
				buffer[DIRECTION_UP  ].SetMaxBufferSize(BUFFER_U_LAYER_0);
				break;
			case 1:
				for (int i = 0; i < 4; i++)
					buffer[i].SetMaxBufferSize(BUFFER_L_LAYER_1);
				buffer[DIRECTION_DOWN].SetMaxBufferSize(BUFFER_D_LAYER_1);
				buffer[DIRECTION_UP  ].SetMaxBufferSize(BUFFER_U_LAYER_1);
				break;
			case 2:
				for (int i = 0; i < 4; i++)
					buffer[i].SetMaxBufferSize(BUFFER_L_LAYER_2);
				buffer[DIRECTION_DOWN].SetMaxBufferSize(BUFFER_D_LAYER_2);
				buffer[DIRECTION_UP  ].SetMaxBufferSize(BUFFER_U_LAYER_2);
				break;
			case 3:
				for (int i = 0; i < 4; i++)
					buffer[i].SetMaxBufferSize(BUFFER_L_LAYER_3);
				buffer[DIRECTION_DOWN].SetMaxBufferSize(BUFFER_D_LAYER_3);
				buffer[DIRECTION_UP  ].SetMaxBufferSize(BUFFER_U_LAYER_3);
				break;
		}

	}*/
}

void NoximRouter::TBDB(float consumption_rate)
{       NoximCoord local = id2Coord(local_id);
 
	float Temp_diff_east  = (stats.pre_temperature1 - stats.temperature) - PDT_neighbor[DIRECTION_EAST].read();
	float Temp_diff_west  = (stats.pre_temperature1 - stats.temperature) - PDT_neighbor[DIRECTION_WEST].read();
	float Temp_diff_north = (stats.pre_temperature1 - stats.temperature) - PDT_neighbor[DIRECTION_NORTH].read();
	float Temp_diff_south = (stats.pre_temperature1 - stats.temperature) - PDT_neighbor[DIRECTION_SOUTH].read();
	float Temp_diff_up    = (stats.pre_temperature1 - stats.temperature) - PDT_neighbor[DIRECTION_UP].read();
	float Temp_diff_down  = (stats.pre_temperature1 - stats.temperature) - PDT_neighbor[DIRECTION_DOWN].read();
/*
	if(local.x == 4 && local.y == 4 ){    //traffic analysis
		buffer[DIRECTION_WEST].SetMaxBufferSize(2);
	}

	if(local.x == 3 && local.y == 3 ){    //traffic analysis
                buffer[DIRECTION_SOUTH].SetMaxBufferSize(2);
        }

	if(local.x == 2 && local.y == 4 ){    //traffic analysis
                buffer[DIRECTION_EAST].SetMaxBufferSize(2);
        }

	if(local.x == 3 && local.y == 5 ){    //traffic analysis
                buffer[DIRECTION_NORTH].SetMaxBufferSize(2);
        }
	
	if(local.x == 3 && local.y == 4 ){    //traffic analysis
		
		buffer[DIRECTION_NORTH].SetMaxBufferSize(6);
                buffer[DIRECTION_WEST].SetMaxBufferSize(6);
		buffer[DIRECTION_SOUTH].SetMaxBufferSize(6);
		buffer[DIRECTION_EAST].SetMaxBufferSize(6);
        }
*/
/*
	float Tempdiff[6] = {Temp_diff_east, Temp_diff_west, Temp_diff_north, Temp_diff_south, Temp_diff_up, Temp_diff_down};
	bubbleSort(Tempdiff, 6);
	for(int j = 0; j < 6; j++) {
		buffer[DIRECTION_SOUTH].SetMaxBufferSize(2);
        }
*/
/*
	int east = 0;
	int west = 0;
	int north = 0;
	int south = 0;
	int up = 0;
	int down = 0;

        if(Temp_diff_east > Temp_diff_west){
		east++;
		west--;
	}
	else{
		east--;
		west++;
	}

	if(Temp_diff_east > Temp_diff_north){
		east++;
		north--;
	}
	else{
		east--;
		north++;
	}

	if(Temp_diff_east > Temp_diff_south){
                east++;
                south--;
        }
        else{
                east--;
                south++;
        }

	if(Temp_diff_east > Temp_diff_up){
                east++;
                up--;
        }
        else{
                east--;
                up++;
        }

	if(Temp_diff_east > Temp_diff_down){
                east++;
                down--;
        }
        else{
                east--;
                down++;
        }
//-----------------
	if(Temp_diff_west > Temp_diff_north){
                west++;
                north--;
        }
        else{
                west--;
                north++;
        }
//----------------
	if(Temp_diff_west > Temp_diff_south){
                west++;
                south--;
        }
        else{
                west--;
                south++;
        }
//---------------
	if(Temp_diff_west > Temp_diff_up){
                west++;
                up--;
        }
        else{
                west--;
                up++;
        }
//----------------
	if(Temp_diff_west > Temp_diff_down){
                west++;
                down--;
        }
        else{
                west--;
                down++;
        }

//----------------
//----------------

	if(Temp_diff_north > Temp_diff_south){
                north++;
                south--;
        }
        else{
                north--;
                south++;
        }

//----------------
//----------------

        if(Temp_diff_north > Temp_diff_up){
                north++;
                up--;
        }
        else{
                north--;
                up++;
        }

//----------------
//----------------

        if(Temp_diff_north > Temp_diff_down){
                north++;
                down--;
        }
        else{
                north--;
                down++;
        }

//---------------
//---------------
//---------------

	if(Temp_diff_south > Temp_diff_down){
                south++;
                down--;
        }
        else{
                south--;
                down++;
        }

//---------------
//---------------
//---------------

        if(Temp_diff_south > Temp_diff_up){
                south++;
                up--;
        }
        else{
                south--;
                up++;
        }

//---------------
//---------------
//---------------
//---------------

        if(Temp_diff_up > Temp_diff_down){
                up++;
                down--;
        }
        else{
                up--;
                down++;
        }

if(stats.pre_temperature1 > 98){
	if(east==5)
        	buffer[DIRECTION_EAST].SetMaxBufferSize(7);
	else if(east==3)
		buffer[DIRECTION_EAST].SetMaxBufferSize(6);
	else if(east==1)
                buffer[DIRECTION_EAST].SetMaxBufferSize(5);
	else if(east==-1)
                buffer[DIRECTION_EAST].SetMaxBufferSize(4);
	else if(east==-3)
                buffer[DIRECTION_EAST].SetMaxBufferSize(3);
	else if(east==-5)
                buffer[DIRECTION_EAST].SetMaxBufferSize(2);

	
	if(south==5)
                buffer[DIRECTION_SOUTH].SetMaxBufferSize(7);
        else if(south==3)
                buffer[DIRECTION_SOUTH].SetMaxBufferSize(6);
        else if(south==1)
                buffer[DIRECTION_SOUTH].SetMaxBufferSize(5);
        else if(south==-1)
                buffer[DIRECTION_SOUTH].SetMaxBufferSize(4);
        else if(south==-3)
                buffer[DIRECTION_SOUTH].SetMaxBufferSize(3);
        else if(south==-5)
                buffer[DIRECTION_SOUTH].SetMaxBufferSize(2);



	if(west==5)
                buffer[DIRECTION_WEST].SetMaxBufferSize(7);
        else if(west==3)
                buffer[DIRECTION_WEST].SetMaxBufferSize(6);
        else if(west==1)
                buffer[DIRECTION_WEST].SetMaxBufferSize(5);
        else if(west==-1)
                buffer[DIRECTION_WEST].SetMaxBufferSize(4);
        else if(west==-3)
                buffer[DIRECTION_WEST].SetMaxBufferSize(3);
        else if(west==-5)
                buffer[DIRECTION_WEST].SetMaxBufferSize(2);

	
	if(north==5)
                buffer[DIRECTION_NORTH].SetMaxBufferSize(7);
        else if(north==3)
                buffer[DIRECTION_NORTH].SetMaxBufferSize(6);
        else if(north==1)
                buffer[DIRECTION_NORTH].SetMaxBufferSize(5);
        else if(north==-1)
                buffer[DIRECTION_NORTH].SetMaxBufferSize(4);
        else if(north==-3)
                buffer[DIRECTION_NORTH].SetMaxBufferSize(3);
        else if(north==-5)
                buffer[DIRECTION_NORTH].SetMaxBufferSize(2);


	if(up==5)
                buffer[DIRECTION_UP].SetMaxBufferSize(7);
        else if(up==3)
                buffer[DIRECTION_UP].SetMaxBufferSize(6);
        else if(up==1)
                buffer[DIRECTION_UP].SetMaxBufferSize(5);
        else if(up==-1)
                buffer[DIRECTION_UP].SetMaxBufferSize(4);
        else if(up==-3)
                buffer[DIRECTION_UP].SetMaxBufferSize(3);
        else if(up==-5)
                buffer[DIRECTION_UP].SetMaxBufferSize(2);


	if(down==5)
                buffer[DIRECTION_DOWN].SetMaxBufferSize(7);
        else if(down==3)
                buffer[DIRECTION_DOWN].SetMaxBufferSize(6);
        else if(down==1)
                buffer[DIRECTION_DOWN].SetMaxBufferSize(5);
        else if(down==-1)
                buffer[DIRECTION_DOWN].SetMaxBufferSize(4);
        else if(down==-3)
                buffer[DIRECTION_DOWN].SetMaxBufferSize(3);
        else if(down==-5)
                buffer[DIRECTION_DOWN].SetMaxBufferSize(2);

}*/

                if(Temp_diff_east<0){
                	if(buffer[DIRECTION_EAST].GetMaxBufferSize()>2){
                        	buffer[DIRECTION_EAST].SetMaxBufferSize(buffer[DIRECTION_EAST].GetMaxBufferSize()-1);
				buf_budget++;
			}
        	}
		if(Temp_diff_west<0){
        	        if(buffer[DIRECTION_WEST].GetMaxBufferSize()>2){
                	        buffer[DIRECTION_WEST].SetMaxBufferSize(buffer[DIRECTION_WEST].GetMaxBufferSize()-1);
				buf_budget++;
			}
       		}
		if(Temp_diff_north<0){
                	if(buffer[DIRECTION_NORTH].GetMaxBufferSize()>2){
                        	buffer[DIRECTION_NORTH].SetMaxBufferSize(buffer[DIRECTION_NORTH].GetMaxBufferSize()-1);
				buf_budget++;
			}
        	}
		if(Temp_diff_south<0){
                	if(buffer[DIRECTION_SOUTH].GetMaxBufferSize()>2){
                        	buffer[DIRECTION_SOUTH].SetMaxBufferSize(buffer[DIRECTION_SOUTH].GetMaxBufferSize()-1);
				buf_budget++;
			}
        	}	
		if(Temp_diff_up<0){
                	if(buffer[DIRECTION_UP].GetMaxBufferSize()>2){
                        	buffer[DIRECTION_UP].SetMaxBufferSize(buffer[DIRECTION_UP].GetMaxBufferSize()-1);
				buf_budget++;
			}
        	}
		if(Temp_diff_down<0){
                	if(buffer[DIRECTION_DOWN].GetMaxBufferSize()>2){
                        	buffer[DIRECTION_DOWN].SetMaxBufferSize(buffer[DIRECTION_DOWN].GetMaxBufferSize()-1);
				buf_budget++;
			}
        	}


	        if(Temp_diff_east>0 && buf_budget > 0){
	                if(buffer[DIRECTION_EAST].GetMaxBufferSize()<8){
        	                buffer[DIRECTION_EAST].SetMaxBufferSize(buffer[DIRECTION_EAST].GetMaxBufferSize()+1);
				buf_budget--;
			}
        	}
		if(Temp_diff_west>0 && buf_budget > 0){
                	if(buffer[DIRECTION_WEST].GetMaxBufferSize()<8){
                        	buffer[DIRECTION_WEST].SetMaxBufferSize(buffer[DIRECTION_WEST].GetMaxBufferSize()+1);
				buf_budget--;
			}
        	}
		if(Temp_diff_north>0 && buf_budget > 0){
                        if(buffer[DIRECTION_NORTH].GetMaxBufferSize()<8){
                                buffer[DIRECTION_NORTH].SetMaxBufferSize(buffer[DIRECTION_NORTH].GetMaxBufferSize()+1);
                                buf_budget--;
                        }
                }
                if(Temp_diff_south>0 && buf_budget > 0){
                        if(buffer[DIRECTION_SOUTH].GetMaxBufferSize()<8){
                                buffer[DIRECTION_SOUTH].SetMaxBufferSize(buffer[DIRECTION_SOUTH].GetMaxBufferSize()+1);
                                buf_budget--;
                        }
                }
		if(Temp_diff_up>0 && buf_budget > 0){
                        if(buffer[DIRECTION_UP].GetMaxBufferSize()<8){
                                buffer[DIRECTION_UP].SetMaxBufferSize(buffer[DIRECTION_UP].GetMaxBufferSize()+1);
                                buf_budget--;
                        }
                }
                if(Temp_diff_down>0 && buf_budget > 0){
                        if(buffer[DIRECTION_DOWN].GetMaxBufferSize()<8){
                                buffer[DIRECTION_DOWN].SetMaxBufferSize(buffer[DIRECTION_DOWN].GetMaxBufferSize()+1);
                                buf_budget--;
                        }
                }
		
/*
	if(Temp_diff_east<0){
		if(buffer[DIRECTION_EAST].GetMaxBufferSize()>2)
			buffer[DIRECTION_EAST].SetMaxBufferSize(buffer[DIRECTION_EAST].GetMaxBufferSize()-1);
	}
	if(Temp_diff_east>0){
		if(buffer[DIRECTION_EAST].GetMaxBufferSize()<8)
			buffer[DIRECTION_EAST].SetMaxBufferSize(buffer[DIRECTION_EAST].GetMaxBufferSize()+1);
	}
	if(Temp_diff_west<0){
                if(buffer[DIRECTION_WEST].GetMaxBufferSize()>2)
                        buffer[DIRECTION_WEST].SetMaxBufferSize(buffer[DIRECTION_WEST].GetMaxBufferSize()-1);
	}
       	if(Temp_diff_west>0){
                if(buffer[DIRECTION_WEST].GetMaxBufferSize()<8)
                        buffer[DIRECTION_WEST].SetMaxBufferSize(buffer[DIRECTION_WEST].GetMaxBufferSize()+1);
	}
	if(Temp_diff_north<0){
                if(buffer[DIRECTION_NORTH].GetMaxBufferSize()>2)
                        buffer[DIRECTION_NORTH].SetMaxBufferSize(buffer[DIRECTION_NORTH].GetMaxBufferSize()-1);
	}
        if(Temp_diff_north>0){
                if(buffer[DIRECTION_NORTH].GetMaxBufferSize()<8)
                        buffer[DIRECTION_NORTH].SetMaxBufferSize(buffer[DIRECTION_NORTH].GetMaxBufferSize()+1);
	}
	if(Temp_diff_south<0){
                if(buffer[DIRECTION_SOUTH].GetMaxBufferSize()>2)
                        buffer[DIRECTION_SOUTH].SetMaxBufferSize(buffer[DIRECTION_SOUTH].GetMaxBufferSize()-1);
	}
        if(Temp_diff_south>0){
                if(buffer[DIRECTION_SOUTH].GetMaxBufferSize()<8)
                        buffer[DIRECTION_SOUTH].SetMaxBufferSize(buffer[DIRECTION_SOUTH].GetMaxBufferSize()+1);
	}
	if(Temp_diff_up<0){
                if(buffer[DIRECTION_UP].GetMaxBufferSize()>2)
                        buffer[DIRECTION_UP].SetMaxBufferSize(buffer[DIRECTION_UP].GetMaxBufferSize()-1);
	}
        if(Temp_diff_up>0){
                if(buffer[DIRECTION_UP].GetMaxBufferSize()<8)
                        buffer[DIRECTION_UP].SetMaxBufferSize(buffer[DIRECTION_UP].GetMaxBufferSize()+1);
	}
	if(Temp_diff_down<0){
                if(buffer[DIRECTION_DOWN].GetMaxBufferSize()>2)
                        buffer[DIRECTION_DOWN].SetMaxBufferSize(buffer[DIRECTION_DOWN].GetMaxBufferSize()-1);
	}
        if(Temp_diff_down>0){
                if(buffer[DIRECTION_DOWN].GetMaxBufferSize()<8)
                        buffer[DIRECTION_DOWN].SetMaxBufferSize(buffer[DIRECTION_DOWN].GetMaxBufferSize()+1);
	}

*/

/*
	cout<<"Router:"<<local.x<<local.y<<local.z;
	cout<<"Temp_diff_east:"<<Temp_diff_east<<" buf_east:"<<buffer[DIRECTION_EAST].GetMaxBufferSize()<<endl;
	
	cout<<"Router:"<<local.x<<local.y<<local.z;
        cout<<"Temp_diff_west:"<<Temp_diff_west<<" buf_west:"<<buffer[DIRECTION_WEST].GetMaxBufferSize()<<endl;

	cout<<"Router:"<<local.x<<local.y<<local.z;
        cout<<"Temp_diff_north:"<<Temp_diff_north<<" buf_north:"<<buffer[DIRECTION_NORTH].GetMaxBufferSize()<<endl;

	cout<<"Router:"<<local.x<<local.y<<local.z;
        cout<<"Temp_diff_south:"<<Temp_diff_south<<" buf_south:"<<buffer[DIRECTION_SOUTH].GetMaxBufferSize()<<endl;

	cout<<"Router:"<<local.x<<local.y<<local.z;
        cout<<"Temp_diff_up:"<<Temp_diff_up<<" buf_up:"<<buffer[DIRECTION_UP].GetMaxBufferSize()<<endl;

	cout<<"Router:"<<local.x<<local.y<<local.z;
        cout<<"Temp_diff_down:"<<Temp_diff_down<<" buf_down:"<<buffer[DIRECTION_DOWN].GetMaxBufferSize()<<end	
	if(MTTT[local.x][local.y][local.z] < 2){
		
                int buffer_dep;

                buffer_dep = ceil(NoximGlobalParams::buffer_depth*MTTT[local.x][local.y][local.z]);
                if(buffer_dep > NoximGlobalParams::buffer_depth)
                        buffer_dep =  NoximGlobalParams::buffer_depth;
                else if(buffer_dep < 1)
                        buffer_dep = 1;

                for (int i = 0; i < DIRECTIONS+2; i++)
                           buffer[i].SetMaxBufferSize(buffer_dep);
		
		for (int i = 0; i < DIRECTIONS+2; i++)
                        if(buffer[i].GetMaxBufferSize()>2)
                           buffer[i].SetMaxBufferSize(buffer[i].GetMaxBufferSize()-2);
        }
	else{
		for (int i = 0; i < DIRECTIONS+2; i++)
                        if(buffer[i].GetMaxBufferSize()<8)
                           buffer[i].SetMaxBufferSize(buffer[i].GetMaxBufferSize()+2);
	
	}
	*/

	

}

unsigned long NoximRouter::getRoutedFlits()
{
	unsigned long total = 0 ; 
	for (int i = 0; i < DIRECTIONS + 1; i++)
		total += routed_flits[i];
    return total;
}

unsigned long NoximRouter::getRoutedDWFlits()
{
	return routed_DWflits;
}

unsigned long NoximRouter::getRoutedFlits(int i)
{
    return routed_flits[i];
}

unsigned long NoximRouter::getWaitingTime(int i)
{
	return waiting[i];
}
unsigned long NoximRouter::getTotalWaitingTime()
{
	return _total_waiting;
} 
unsigned long NoximRouter::getRoutedPackets()
{
	return routed_packets;
}
void NoximRouter::CalcDelay(){
	_buffer_pkt_count = 0;
	_buffer_pkt_nw_delay = 0;
	_buffer_pkt_ni_delay = 0;
	_buffer_pkt_msg_delay= 0;
	for (int i = 0; i < DIRECTIONS + 1; i++){
		while( !buffer[i].IsEmpty() ){
			NoximFlit flit = buffer[i].Front();
			if( flit.flit_type == FLIT_TYPE_HEAD ){
				_buffer_pkt_msg_delay += (int)getCurrentCycleNum() - (int)flit.timestamp    ;
				_buffer_pkt_ni_delay  += (int)getCurrentCycleNum() - (int)flit.timestamp_ni ;
				_buffer_pkt_nw_delay  += (int)getCurrentCycleNum() - (int)flit.timestamp_nw ;
				_buffer_pkt_count     ++;
			}
			buffer[i].Pop();
		}
	}
}

int NoximRouter::getFlitRoute(int i){
	NoximFlit flit = buffer[i].Front();
	NoximRouteData route_data;
	route_data.current_id = local_id;
	route_data.src_id     = (flit.arr_mid)?flit.mid_id:flit.src_id;
	route_data.dst_id     = (flit.arr_mid)?flit.dst_id:flit.mid_id;
	route_data.dir_in     = i;
	route_data.routing    = flit.routing_f;
	route_data.DW_layer   = flit.DW_layer;
	route_data.arr_mid    = flit.arr_mid;

	return route(route_data);
}

int NoximRouter::getDirAvailable(int i){
	
	return reservation_table.isAvailable(i);
	
}

double NoximRouter::getPower()
{
    //return stats.power.getPower();
    //return stats.power.getTransientRouterPower();
    return stats.power.getSteadyStateRouterPower();
}

int NoximRouter::reflexDirection(int direction) const
{
    switch( direction ){
	case DIRECTION_NORTH:return DIRECTION_SOUTH;break;
    case DIRECTION_EAST :return DIRECTION_WEST ;break;
    case DIRECTION_WEST :return DIRECTION_EAST ;break;
    case DIRECTION_SOUTH:return DIRECTION_NORTH;break;
	case DIRECTION_UP   :return DIRECTION_DOWN ;break;
    case DIRECTION_DOWN :return DIRECTION_UP   ;break;
	default             :return NOT_VALID      ;break;
	}
}

int NoximRouter::getNeighborId(int _id, int direction) const
{
    NoximCoord my_coord = id2Coord(_id);
    switch ( direction ) {
    case DIRECTION_NORTH: if (my_coord.y == 0                                ) return NOT_VALID;my_coord.y--;break;
    case DIRECTION_SOUTH: if (my_coord.y == NoximGlobalParams::mesh_dim_y - 1) return NOT_VALID;my_coord.y++;break;
    case DIRECTION_EAST : if (my_coord.x == NoximGlobalParams::mesh_dim_x - 1) return NOT_VALID;my_coord.x++;break;
    case DIRECTION_WEST : if (my_coord.x == 0                                ) return NOT_VALID;my_coord.x--;break;
	case DIRECTION_UP   : if (my_coord.z == 0                                ) return NOT_VALID;my_coord.z--;break;
	case DIRECTION_DOWN : if (my_coord.z == NoximGlobalParams::mesh_dim_z-1  ) return NOT_VALID;my_coord.z++;break;
    default             : cout << "direction not valid : " << direction; assert(false);
    }
    return coord2Id(my_coord);
}

void NoximRouter::TraffThrottlingProcess()	//若在emergency mode, 且traffic超過traffic quota, 則throttle
{

	NoximCoord local = id2Coord(local_id);
	if( reset.read() )for(int i=0; i<DIRECTIONS; i++){
		on_off[i].write(0);
		TB[i].write(0);
		buf[0][i].write(0);
		buf[1][i].write(0);
		buf[2][i].write(0);
		buf[3][i].write(0);
		buf[4][i].write(0);
		buf[5][i].write(0);
		buf[6][i].write(0);
		buf[7][i].write(0);
		RST = 0;
		DFS = 8;
		//free_slots_PE[i].write( free_slots_neighbor[i]);
	}
	else              for(int i=0; i<DIRECTIONS; i++){
		on_off[i].write( _emergency );
		TB[i].write(MTTT[local.x][local.y][local.z]);
		PDT[i].write(stats.pre_temperature1-stats.temperature);
		if(throttling[local.x][local.y][local.z]){
			buf[0][i].write(100);
			buf[1][i].write(100);
			buf[2][i].write(100);
			buf[3][i].write(100);
			buf[4][i].write(100);
                        buf[5][i].write(100);
                        buf[6][i].write(100);
                        buf[7][i].write(100);
		}else{
		/*	
			if(MTTT[local.x][local.y][local.z] < 0.5 && MTTT[local.x][local.y][local.z] != 0){
                        	if(RST<4)
                        	        RST++;
                	}
                	else{
                        	if(RST>0)
                                	RST--;
                	}
		*/
		//if(buffer[DIRECTION_SOUTH].IsEmpty())
		//	buf[0][i].write(RST + buffer[DIRECTION_SOUTH].GetMaxBufferSize());
		//else
			buf[0][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_SOUTH].Size());//buffer[DIRECTION_SOUTH].GetMaxBufferSize());
		
		//if(buffer[DIRECTION_WEST].IsEmpty())
		//	buf[1][i].write(RST + buffer[DIRECTION_WEST].GetMaxBufferSize());
		//else
			buf[1][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_WEST].Size());//buffer[DIRECTION_WEST].GetMaxBufferSize());

		//if(buffer[DIRECTION_NORTH].IsEmpty())
		//	buf[2][i].write(RST + buffer[DIRECTION_NORTH].GetMaxBufferSize());
		//else
			buf[2][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_NORTH].Size());//buffer[DIRECTION_NORTH].GetMaxBufferSize());

		//if(buffer[DIRECTION_EAST].IsEmpty())
		//	buf[3][i].write(RST + buffer[DIRECTION_EAST].GetMaxBufferSize());
		//else
			buf[3][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_EAST].Size());//buffer[DIRECTION_EAST].GetMaxBufferSize());

		//if(buffer[DIRECTION_DOWN].IsEmpty())
		//	buf[4][i].write(RST + buffer[DIRECTION_DOWN].GetMaxBufferSize());
		//else
			buf[4][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_DOWN].Size());//buffer[DIRECTION_DOWN].GetMaxBufferSize());

		//if(buffer[DIRECTION_UP].IsEmpty())
		//	buf[5][i].write(RST + buffer[DIRECTION_UP].GetMaxBufferSize());
		//else
			buf[5][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_UP].Size());//buffer[DIRECTION_UP].GetMaxBufferSize());

		//if(buffer[DIRECTION_LOCAL].IsEmpty())
		//	buf[6][i].write(RST + buffer[DIRECTION_LOCAL].GetMaxBufferSize());
		//else
			buf[6][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_LOCAL].Size());//buffer[DIRECTION_LOCAL].GetMaxBufferSize());

		//if(buffer[DIRECTION_SEMI_LOCAL].IsEmpty())
		//	buf[7][i].write(RST + buffer[DIRECTION_SEMI_LOCAL].GetMaxBufferSize());
		//else
			buf[7][i].write((1+RST*0.125) + (1+RST*0.125)*buffer[DIRECTION_SEMI_LOCAL].Size());//buffer[DIRECTION_SEMI_LOCAL].GetMaxBufferSize());

		}

		/*		
		if(MTTT[local.x][local.y][local.z] < 0.5 && MTTT[local.x][local.y][local.z] != 0){
			if(RST<4)
				RST++;
		}
		else{
			if(RST>0)
				RST--;
		}
		*/
		//free_slots_PE[i].write( free_slots_neighbor[i]);
	}
	//for(int i=0; i<8; i++)
	//RCA_PE[i] = RCA_data_in[i];
	// free_slots_PE[i].write( free_slots_neighbor[i]);
}

void NoximRouter::IntoEmergency(){
	_emergency  = true;
	if(_emergency_level < NoximGlobalParams::buffer_depth-1)
		_emergency_level++;
}

void NoximRouter::OutOfEmergency(){
	_emergency       = false;
	if(_emergency_level > 0)
		_emergency_level--;
}

bool NoximRouter::inCongestion()
{
    for (int i = 0; i < DIRECTIONS; i++) {
	int flits =        NoximGlobalParams::buffer_depth - free_slots_neighbor[i];
	if (flits > (int) (NoximGlobalParams::buffer_depth * NoximGlobalParams::dyad_threshold) )return true;
    }
    return false;
}
bool NoximRouter::Adaptive_ok( NoximCoord & sour, NoximCoord & dest)
{
	int x_diff = dest.x - sour.x;
	int y_diff = dest.y - sour.y;
	int z_diff = dest.z - sour.z;
    int x_search,y_search;

	for( int y_a = 0 ; y_a < abs(y_diff) + 1 ; y_a ++ )
	for( int x_a = 0 ; x_a < abs(x_diff) + 1 ; x_a ++ ){
		x_search = (x_diff > 0)?(sour.x + x_a):(sour.x - x_a);
		y_search = (y_diff > 0)?(sour.y + y_a):(sour.y - y_a);
		if( throttling[x_search][y_search][sour.z] == 1 ) 
		   return false;
	}
	return true;
}
int inRing(NoximCoord dest){//Find the ring level of destination
	int a = dest.x;
	int b = NoximGlobalParams::mesh_dim_x - dest.x;
	int c = dest.y;
	int d = NoximGlobalParams::mesh_dim_y - dest.y;
	int e = ( a < b )?a:b;
	int f = ( c < d )?c:d;
	int g = ( e < f )?e:f;
	return g;
}
void  NoximRouter::DBA( int outgoing,NoximFlit* head){
	NoximCoord local = id2Coord(local_id);
	NoximCoord cascade_node = id2Coord(head->mid_id);
	int cascade_node_ring = inRing(cascade_node);
	int ring = inRing(local);
	if ( cascade_node_ring ==0 && ring != 0 && outgoing != DIRECTION_EAST && outgoing != DIRECTION_WEST){
		
		if( cascade_node.x == 0 )
			cascade_node.x += ring;
		else 
			cascade_node.x -= ring;
		if( cascade_node.x == 0 )
			cascade_node.y += ring;
		else 
			cascade_node.y -= ring;
		if ( Adaptive_ok( local, cascade_node) )
			head->mid_id = coord2Id(cascade_node);
	}
}
