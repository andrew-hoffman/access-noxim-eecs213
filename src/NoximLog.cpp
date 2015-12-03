#include "NoximLog.h"
#include <sys/stat.h>
using namespace std;
extern ofstream results_log_pwr;
extern ofstream transient_log_throughput;
extern ofstream transient_topology;
void NoximLog::BufferLog( NoximNoC *n ){
	ofstream results_buffer;
	if(!mkdir("results/buffer",0777)) cout<<"Making new directory results/buffer"<<endl;
	string buffer_filename = string("results/buffer/buffer");
	buffer_filename = MarkFileName( buffer_filename);
	results_buffer.open(buffer_filename.c_str(),ios::out);
	if(!results_buffer.is_open())
		cout<<"Cannot open"<< buffer_filename.c_str() <<endl;
	else{
		//Print IDs
		results_buffer<<"IDs"<<endl;
		for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){
			results_buffer<<"Layer "<<z<<endl;
			for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){
				for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++)
					results_buffer<<n->t[x][y][z]->r->local_id<<"\t";
				results_buffer<<"\n";	
			}	
		}
		results_buffer<<"\n";
		//Print free slots of buffers in router
		results_buffer<<"freeslots"<<endl;
		for(int d = 0; d < DIRECTIONS+1; d++){
			results_buffer<<"Directions "<<d<<endl;
			for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){
				results_buffer<<"Layer "<<z<<endl;
				for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){
					for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++)
						results_buffer<<n->t[x][y][z]->r->buffer[d].getCurrentFreeSlots()<<"\t";
					results_buffer<<"\n";	
				}
			}
			results_buffer<<"\n";
		}
		//Print waiting time of head flit in each buffer
		results_buffer<<"wait time"<<endl;
		for(int d = 0; d < DIRECTIONS+1; d++){
			results_buffer<<"Directions "<<d<<endl;
			for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){
				results_buffer<<"Layer "<<z<<endl;
				for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){ 
					for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++){
						if ( !(n->t[x][y][z]->r->buffer[d].IsEmpty()) ){
							/*
							NoximFlit flit = n->t[x][y][z]->r->buffer[d].Front();
							double tt= 0;
							tt= NoximGlobalParams::simulation_time + NoximGlobalParams::stats_warm_up_time - flit.timestamp;
							results_buffer<<tt<<"\t";
							*/
							results_buffer<<n->t[x][y][z]->r->getWaitingTime(d)<<"\t";
						}
						else
							results_buffer<<0<<"\t";
					}
					results_buffer<<"\n";
				}
			}
			results_buffer<<"\n";
		}
		
		int packet_in_buffer = 0;
		results_buffer<<"dst_id"<<endl;
		for(int d = 0; d < DIRECTIONS+1; d++){
			results_buffer<<"Directions "<<d<<endl;
			for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){
				results_buffer<<"Layer "<<z<<endl;
				for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){
					for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++){
						if ( !(n->t[x][y][z]->r->buffer[d].IsEmpty()) ){
							NoximFlit flit = n->t[x][y][z]->r->buffer[d].Front();
							results_buffer<<flit.dst_id<<"\t";
							packet_in_buffer++;
						}
						else
							results_buffer<<"na"<<"\t";
					}
					results_buffer<<"\n";
				}
			}
			results_buffer<<"\n";
		}
  
		results_buffer<<"src_id"<<endl;
		for(int d = 0; d < DIRECTIONS+1; d++){	
			results_buffer<<"Directions "<<d<<endl;
			for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){
				results_buffer<<"Layer "<<z<<endl;
				for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){
					for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++){
						if ( !(n->t[x][y][z]->r->buffer[d].IsEmpty()) ){
							NoximFlit flit = n->t[x][y][z]->r->buffer[d].Front();
							results_buffer<<flit.src_id<<"\t";
						}
						else
							results_buffer<<"na"<<"\t";
					}
					results_buffer<<"\n";
				}
			}
			results_buffer<<"\n";
		}
    
		results_buffer<<"Flit DW Type"<<endl;
		for(int d = 0; d < DIRECTIONS+1; d++){
			results_buffer<<"Directions "<<d<<endl;
			for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){
				results_buffer<<"Layer "<<z<<endl;
				for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){
					for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++){
						if ( !(n->t[x][y][z]->r->buffer[d].IsEmpty()) ){
							NoximFlit flit = n->t[x][y][z]->r->buffer[d].Front();
							results_buffer<<flit.routing_f<<"\t";
						}
						else
						results_buffer<<"na"<<"\t";
					}
					results_buffer<<"\n";
				}
			}
			results_buffer<<"\n";
		}
		
		results_buffer<<"reservatable table"<<endl;
		for(int d = 0; d < DIRECTIONS+1; d++){
			results_buffer<<"Directions "<<d<<endl;
			for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){ 
				results_buffer<<"Layer "<<z<<endl;
				for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){
					for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++)
						results_buffer<<n->t[x][y][z]->r->reservation_table.getOutputPort(d)<<"\t";
					results_buffer<<"\n";
				}
			}
			results_buffer<<"\n";
		}
  
		results_buffer<<"on_off"<<endl;
		for(int d = 0; d < 4; d++){
			results_buffer<<"Directions "<<d<<endl;
			for(int z=0; z < NoximGlobalParams::mesh_dim_z; z++){ 
				results_buffer<<"Layer "<<z<<endl;
				for(int y=0; y < NoximGlobalParams::mesh_dim_y; y++){
					for(int x=0; x < NoximGlobalParams::mesh_dim_x; x++)
						results_buffer<<n->t[x][y][z]->r->on_off_neighbor[d]<<"\t";
					results_buffer<<"\n";
				}
			}
			results_buffer<<"\n";
		}
  
		int total_transmit             =0;
		int total_not_transmit         =0;
		int total_adaptive_transmit    =0;
		int total_dor_transmit         =0;
		int total_dw_transmit          =0;
		int total_mid_adaptive_transmit=0;
		int total_mid_dor_transmit     =0;
		int total_mid_dw_transmit      =0;
		int total_beltway              =0;

		for(int z = 0; z < NoximGlobalParams::mesh_dim_z; z++)
		for(int y = 0; y < NoximGlobalParams::mesh_dim_y; y++)
		for(int x = 0; x < NoximGlobalParams::mesh_dim_x; x++){
			total_transmit         += n->t[x][y][z]->pe->transmit;
			total_not_transmit     += n->t[x][y][z]->pe->not_transmit;
			total_adaptive_transmit+= n->t[x][y][z]->pe->getAdaptive_Transmit();
			total_dor_transmit     += n->t[x][y][z]->pe->getDOR_Transmit();
			total_dw_transmit      += n->t[x][y][z]->pe->getDW_Transmit();
			total_mid_adaptive_transmit+= n->t[x][y][z]->pe->getMid_Adaptive_Transmit();
			total_mid_dor_transmit     += n->t[x][y][z]->pe->getMid_DOR_Transmit();
			total_mid_dw_transmit      += n->t[x][y][z]->pe->getMid_DW_Transmit();
		}
		results_buffer<<"transmit:"<<"\t";
		results_buffer<<total_transmit<<"\n";
		
		results_buffer<<"not_transmit:"<<"\t";
		results_buffer<<total_not_transmit<<"\n";
		
		results_buffer<<"adaptive_transmit:"    <<"\t"<<total_adaptive_transmit<<"\n";
		results_buffer<<"DOR_transmit:"         <<"\t"<<total_dor_transmit     <<"\n";
		results_buffer<<"DW_transmit:"          <<"\t"<<total_dw_transmit      <<"\n";
		results_buffer<<"mid_adaptive_transmit:"<<"\t"<<total_mid_adaptive_transmit<<"\n";
		results_buffer<<"mid_DOR_transmit:"     <<"\t"<<total_mid_dor_transmit     <<"\n";
		results_buffer<<"mid_DW_transmit:"      <<"\t"<<total_mid_dw_transmit      <<"\n";
		
		results_buffer<<"packet_in_buffer"<<"\t";
		results_buffer<<packet_in_buffer/2<<"\n";
		
		results_buffer.close();
	}
}

void NoximLog::TrafficLog( NoximNoC *n ){
	ofstream results_STLD;
	if(!mkdir("results/STLD",0777)) cout<<"Making new directory results/STLD"<<endl;
	string STLD_filename = string("results/STLD/STLD");
	STLD_filename = MarkFileName( STLD_filename );
	results_STLD.open(STLD_filename.c_str(),ios::out);
	if(!results_STLD.is_open()) cout<<"Cannot open"<< STLD_filename.c_str() <<endl;
	else{
		for( int k = 0 ; k < NoximGlobalParams::mesh_dim_z ; k++){
			results_STLD<<"XY"<<k<<" = [\n";
			for( int j = NoximGlobalParams::mesh_dim_y-1 ; j > -2; j-- ){
				for( int i = 0 ; i < NoximGlobalParams::mesh_dim_x ; i++ ){
					if( j != -1)
						results_STLD<<n->t[i][j][k]->r->getRoutedFlits()<<"\t";
					else
						results_STLD<<"0\t";
				}
				results_STLD<<"0\n";
			}
			results_STLD<<"]"<<endl;
		}
		results_STLD<<"color_range = [0 100000];"<<endl;
		results_STLD<<"figure(1)"<<endl;
		int temp = 1;
		for( int k = 0 ; k < NoximGlobalParams::mesh_dim_z ; k++){
			results_STLD<<"subplot("<<NoximGlobalParams::mesh_dim_z<<",1,"<<temp<<"), pcolor(XY"<<k<<"), axis off, caxis( color_range ), colormap(jet)"<<endl;
			temp += 1;
		}
		results_STLD<<"set(gcf, 'PaperPosition', [1 1 7 30]);"<<endl;
		results_STLD<<"print(gcf,'-djpeg','-r0','"<<MarkFileName( string("") )<<".jpg')"<<endl;
		//for total waiting time
		results_STLD<<"total waiting time"<<endl;
		for( int k = 0 ; k < NoximGlobalParams::mesh_dim_z ; k++){
			results_STLD<<"XY"<<k<<" = [\n";
			for( int j = NoximGlobalParams::mesh_dim_y-1 ; j > -2; j-- ){
				for( int i = 0 ; i < NoximGlobalParams::mesh_dim_x ; i++ ){
					if( j != -1)
						results_STLD<<n->t[i][j][k]->r->getTotalWaitingTime()<<"\t";
					else
						results_STLD<<"0\t";
				}
				results_STLD<<"0\n";
			}
			results_STLD<<"]"<<endl;
		}
		results_STLD<<"color_range = [0 100000];"<<endl;
		results_STLD<<"figure(1)"<<endl;
		temp = 1;
		for( int k = 0 ; k < NoximGlobalParams::mesh_dim_z ; k++){
			results_STLD<<"subplot("<<NoximGlobalParams::mesh_dim_z<<",1,"<<temp<<"), pcolor(XY"<<k<<"), axis off, caxis( color_range ), colormap(jet)"<<endl;
			temp += 1;
		}
		
		
		
		
		results_STLD.close();
	}
}

void NoximLog::TraceSignal( NoximNoC *n ){
    if (NoximGlobalParams::trace_mode) {
		tf = sc_create_vcd_trace_file(NoximGlobalParams::trace_filename);
		sc_trace(tf, n->reset, "reset");
		sc_trace(tf, n->clock, "clock");
		int i,j,k;
		for ( i = 0; i < NoximGlobalParams::mesh_dim_x; i++) 
		for ( j = 0; j < NoximGlobalParams::mesh_dim_y; j++) 
		for ( k = 0; k < NoximGlobalParams::mesh_dim_z; k++) {
			char label[30];
			sprintf(label, "req_to_east(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->req_to_east[i][j][k], label);
			sprintf(label, "req_to_west(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->req_to_west[i][j][k], label);
			sprintf(label, "req_to_south(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->req_to_south[i][j][k], label);
			sprintf(label, "req_to_north(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->req_to_north[i][j][k], label);
	    
			sprintf(label, "ack_to_east(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->ack_to_east[i][j][k], label);
			sprintf(label, "ack_to_west(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->ack_to_west[i][j][k], label);
			sprintf(label, "ack_to_south(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->ack_to_south[i][j][k], label);
			sprintf(label, "ack_to_north(%02d)(%02d)(%02d)", i, j,k);
			sc_trace(tf, n->ack_to_north[i][j][k], label);
		}
    }
}

void NoximLog::TraceEnd(){
	sc_close_vcd_trace_file(tf);
}

void NoximLog::PowerLog(){
	if(!mkdir("results/POWER",0777)) cout<<"Making new directory results/POWER"<<endl;
	string pwr_filename = string("results/POWER/PWR");
	pwr_filename = MarkFileName( pwr_filename );
	results_log_pwr.open(pwr_filename.c_str(), ios::out);
	if(!results_log_pwr.is_open())cout<<"Cannot open "<< pwr_filename.c_str() <<endl;
	else{
		for(int z = 0; z < NoximGlobalParams::mesh_dim_z; z++)
		for(int y = 0; y < NoximGlobalParams::mesh_dim_y; y++)
		for(int x = 0; x < NoximGlobalParams::mesh_dim_x; x++){
			results_log_pwr<<"router["<<x<<"]["<<y<<"]["<<z<<"]\t";
			results_log_pwr<<"uP_mem["<<x<<"]["<<y<<"]["<<z<<"]\t";
			results_log_pwr<<"uP_mac["<<x<<"]["<<y<<"]["<<z<<"]\t";
		}
		results_log_pwr<<"\n";
	}
}

void NoximLog::PowerLogEnd(){
	results_log_pwr.close(); 
}

void NoximLog::Throughput   (             ){
	if(!mkdir("results/TransientThroughput",0777)) cout<<"Making new directory results/TransientThroughput"<<endl;
	string throughput_filename = string("results/TransientThroughput/TransThroughput");
	throughput_filename = MarkFileName( throughput_filename );
	transient_log_throughput.open(throughput_filename.c_str(), ios::out);
	if(!transient_log_throughput.is_open())cout<<"Cannot open "<< throughput_filename.c_str() <<endl;
	else{
		transient_log_throughput<<"Cycle \tRecivedFlits \tThroughput \tThrot. Num. \tMax. Temp.\tAdaptive \tDOR \tDW \tMid_Adaptive \tMid_DOR \tMid_DW \tBeltway"<<endl;
	}
	throughput_filename = string("results/TransientThroughput/TransTopology");
	throughput_filename = MarkFileName( throughput_filename );
	transient_topology.open(throughput_filename.c_str(), ios::out);
	if(!transient_topology.is_open())cout<<"Cannot open "<< throughput_filename.c_str() <<endl;
}

void NoximLog::ThroughputEnd(             ){
	transient_log_throughput.close();
	transient_topology.close();
}
