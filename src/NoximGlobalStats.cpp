/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementaton of the global statistics
 */

#include "NoximGlobalStats.h"
#include <sys/stat.h>
using namespace std;
int wait_cnt[200];

NoximGlobalStats::NoximGlobalStats(const NoximNoC * _noc)
{
    noc = _noc;

#ifdef TESTING
    drained_total = 0;
#endif
}

double NoximGlobalStats::getAverageDelay()
{
    unsigned int total_packets = 0;
    double avg_delay = 0.0;
	int x,y,z;
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		unsigned int received_packets = noc->t[x][y][z]->r->stats.getReceivedPackets(); 
		if (received_packets){
			avg_delay += received_packets * noc->t[x][y][z]->r->stats.getAverageDelay();////
			total_packets += received_packets;
		}
	}
    return avg_delay/(double)total_packets;
}

double NoximGlobalStats::getNiAverageDelay()
{
    unsigned int total_packets = 0;
    double avg_delay = 0.0;
	int x,y,z;
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		unsigned int received_packets = noc->t[x][y][z]->r->stats.getReceivedPackets(); 
		if (received_packets){
			avg_delay += received_packets * noc->t[x][y][z]->r->stats.getNiAverageDelay();////
			total_packets += received_packets;
		}
	}
    return avg_delay/(double)total_packets;
}

double NoximGlobalStats::getNwAverageDelay()
{
    unsigned int total_packets = 0;
    double avg_delay = 0.0;
	int x,y,z;
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		unsigned int received_packets = noc->t[x][y][z]->r->stats.getReceivedPackets(); 
		if (received_packets){
			avg_delay += received_packets * noc->t[x][y][z]->r->stats.getNwAverageDelay();////
			total_packets += received_packets;
		}
	}
    return avg_delay/(double)total_packets;
}

vector<double>  NoximGlobalStats::getLayerAverageDelay()
{ 

  vector<double>   avg_delay;
  vector<double>   received_packets;
  vector<double>   total_packets;
	int x,y,z;
	avg_delay       .resize(NoximGlobalParams::mesh_dim_z); 
	received_packets.resize(NoximGlobalParams::mesh_dim_z); 
	total_packets   .resize(NoximGlobalParams::mesh_dim_z); 
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)  
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		received_packets[z] = noc->t[x][y][z]->r->stats.getReceivedPackets();
		if (received_packets[z]){
			avg_delay[z] += received_packets[z] * noc->t[x][y][z]->r->stats.getAverageDelay();////
			total_packets[z] += received_packets[z];
		}
	}
	for ( z = 0 ; z<NoximGlobalParams::mesh_dim_z; z++)  
		avg_delay[z] /= (double)total_packets[z];

return avg_delay;  
}

vector<unsigned int>   NoximGlobalStats::getLayerRoutedFlits(){
	vector<unsigned int>   RoutedFlits;
	int x,y,z;
	RoutedFlits.resize(NoximGlobalParams::mesh_dim_z); 
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)  
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		RoutedFlits[z] += noc->t[x][y][z]->r->getRoutedFlits();
	}

return RoutedFlits;  
}

vector<unsigned int>   NoximGlobalStats::getLayerRoutedFlits(int i){
	vector<unsigned int>   RoutedFlits;
	int x,y,z;
	RoutedFlits.resize(NoximGlobalParams::mesh_dim_z); 
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)  
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		RoutedFlits[z] += noc->t[x][y][z]->r->getRoutedFlits(i);
	}

return RoutedFlits;  
}

double NoximGlobalStats::getAverageDelay(const int src_id,
					 const int dst_id)
{
    NoximTile *tile = noc->searchNode(dst_id);

    assert(tile != NULL);

    return tile->r->stats.getAverageDelay(src_id);
}


double NoximGlobalStats::getDelayHist()
{
//+CHDR delay histogram -----------------------------------------------------
	if(!mkdir("results/Hist",0777)) cout<<"Making new directory results/Hist"<<endl;
	string filename;
	filename = "results/Hist/Hist";
	filename = MarkFileName( filename );
 
	fstream dout;
	dout.open(filename.c_str(),ios::out);
    
	double maxd     = getMaxDelay();
	double interval = 500; 
	int max_dist = NoximGlobalParams::mesh_dim_x+NoximGlobalParams::mesh_dim_y + NoximGlobalParams::mesh_dim_z;
	double *DelayHist = new double[ (unsigned long int)interval+1 ];
	double *DistHist  = new double[ max_dist ];
	//initialize
	for (int i = 0 ; i <= interval ; i++ )
		DelayHist[i]=0;
	for (int i=0;i<max_dist;i++)
		DistHist[i]=0;
	//accumulate delay histogram
	for (int z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for (int y=0; y<NoximGlobalParams::mesh_dim_y; y++)
    for (int x=0; x<NoximGlobalParams::mesh_dim_x; x++)
      {
        NoximCoord coord;
        coord.x = x;
        coord.y = y;
		coord.z = z;
        int node_id = coord2Id(coord);
        getDelayHist(node_id, interval, DistHist, DelayHist);
      }
      
	//-CHDR ---------------------------------------------------------------------
	//matlab code generate
	//print latency histogram
	dout<<"delay=[";
	for (int i=0; i<=interval; i++)
		dout<<DelayHist[i]<<" ";    
	dout<<"];"<<endl;
        
	dout<<"wait_cnt=[";
        for (int i=0; i<=200; i++)
                dout<<wait_cnt[i]<<" ";
        dout<<"];"<<endl;

	//print distance histogram
	dout<<"dist=[";
	for (int i=0; i<max_dist; i++)
		dout<<DistHist[i]<<" ";    
		dout<<"];"<<endl;
  
	dout<<"[M,N]=size(delay);"<<endl
		<<"figure(1);"<<endl
		<<"hold on;"<<endl
		<<"plot([0:10:N*10-10],delay,'s-');"<<endl
		<<"hold off;"<<endl
		<<"figure(2);"<<endl
		<<"[M,N]=size(dist);"<<endl
		<<"hold on;"<<endl
		<<"plot([0:1:N-1],dist,'s-');"<<endl
		<<"hold off;"<<endl;
      
	dout.close();
	return 1.0;
}

//---------------------------------------------------------------------------

double NoximGlobalStats::getDelayHist(const int node_id,double interval, double *DistHist, double *DelayHist)
{
  NoximCoord coord = id2Coord(node_id);

  unsigned int received_packets = noc->t[coord.x][coord.y][coord.z]->r->stats.getReceivedPackets(); 
  if (received_packets)
		return noc->t[coord.x][coord.y][coord.z]->r->stats.getDelayHist(coord,interval,DistHist,DelayHist);
  else
		return -1.0;
}


double NoximGlobalStats::getMaxDelay()
{
    double maxd = -1.0;
	int node_id,x,y;
	double d;
	for (int z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for (int y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for (int x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		NoximCoord coord;
		coord.x = x;
		coord.y = y;
		coord.z = z; 
		node_id = coord2Id(coord);
		d       = getMaxDelay(node_id);
		if (d > maxd)
			maxd = d;
	}
    return maxd;
}

double NoximGlobalStats::getMaxDelay(const int node_id)
{
    NoximCoord coord = id2Coord(node_id);
	unsigned int received_packets = noc->t[coord.x][coord.y][coord.z]->r->stats.getReceivedPackets();
    if (received_packets)
		return noc->t[coord.x][coord.y][coord.z]->r->stats.getMaxDelay();
    else
		return -1.0;
}

double NoximGlobalStats::getNwTotalAverageDelay(){
	double  avg_delay    = 0.0;
	double  total_packets= 0.0;
	for (int z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for (int y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for (int x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		noc->t[x][y][z]->r->CalcDelay();
		total_packets += (noc->t[x][y][z]->r->stats.getReceivedPackets() + noc->t[x][y][z]->r->getFlitsCount() );
		avg_delay += (noc->t[x][y][z]->r->stats.getReceivedPackets() * noc->t[x][y][z]->r->stats.getNwAverageDelay() + noc->t[x][y][z]->r->getNwDelay() );////
		//cout<<"noc->t["<<x<<"]["<<y<<"]["<<z<<"]->r->getFlitsCount()"<<noc->t[x][y][z]->r->getFlitsCount()<<" "<<noc->t[x][y][z]->r->getNwDelay() <<endl;
		
	}
	avg_delay /= total_packets;
	return avg_delay;
}

double NoximGlobalStats::getNiTotalAverageDelay(){
	double  avg_delay    = 0.0;
	double  total_packets= 0.0;
	for (int z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for (int y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for (int x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		noc->t[x][y][z]->pe->CalcDelay();
		total_packets += (noc->t[x][y][z]->r->stats.getReceivedPackets() + noc->t[x][y][z]->r->getFlitsCount() + noc->t[x][y][z]->pe->getFlitsNum() );
		avg_delay += (noc->t[x][y][z]->r->stats.getReceivedPackets() * noc->t[x][y][z]->r->stats.getNiAverageDelay() + noc->t[x][y][z]->r->getNiDelay() + noc->t[x][y][z]->pe->getNiDelay() );////
	}
	avg_delay /= total_packets;
	return avg_delay;
}

double NoximGlobalStats::getMsgTotalAverageDelay(){
	double  avg_delay    = 0.0;
	double  total_packets= 0.0;
	for (int z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for (int y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for (int x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		noc->t[x][y][z]->pe->CalcMessageDelay();
		total_packets += (noc->t[x][y][z]->r->stats.getReceivedPackets() + noc->t[x][y][z]->r->getFlitsCount() + noc->t[x][y][z]->pe->getFlitsNum() + noc->t[x][y][z]->pe->getMsgFlitsNum() );
		avg_delay += (noc->t[x][y][z]->r->stats.getReceivedPackets() * noc->t[x][y][z]->r->stats.getAverageDelay() + noc->t[x][y][z]->r->getMsgDelay() + noc->t[x][y][z]->pe->getPktQMsgDelay() + noc->t[x][y][z]->pe->getMsgQDelay() );////
	}
	avg_delay /= total_packets;
	return avg_delay;
}

double NoximGlobalStats::getMaxDelay(const int src_id, const int dst_id){
    NoximTile *tile = noc->searchNode(dst_id);
    assert(tile != NULL);
    return tile->r->stats.getMaxDelay(src_id);
}

vector<vector<vector<double> > > NoximGlobalStats::getMaxDelayMtx()
{
	vector<vector<vector<double> > > mtx;
	int x,y,z,id;
	mtx.resize(NoximGlobalParams::mesh_dim_z); ////
	for ( z = 0 ; z<NoximGlobalParams::mesh_dim_z; z++){
		mtx[z].resize(NoximGlobalParams::mesh_dim_x * NoximGlobalParams::mesh_dim_y);
		for ( y = 0 ; y<NoximGlobalParams::mesh_dim_y; y++)
		mtx[z][y].resize(NoximGlobalParams::mesh_dim_x);
	}
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		NoximCoord coord;
		coord.x = x;
		coord.y = y;
		coord.z = z;
		id = coord2Id(coord);
		mtx[z][y][x] = getMaxDelay(id);
	}
  return mtx;
}

double NoximGlobalStats::getAverageThroughput(const int src_id,
					      const int dst_id)
{
    NoximTile *tile = noc->searchNode(dst_id);
    assert(tile != NULL);
    return tile->r->stats.getAverageThroughput(src_id);
}

double NoximGlobalStats::getAverageThroughput()
{
    unsigned int total_comms = 0;
    double avg_throughput = 0.0;
	int x,y,z;
	unsigned int ncomms;
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		ncomms = noc->t[x][y][z]->r->stats.getTotalCommunications();
		if (ncomms){
			avg_throughput += ncomms * noc->t[x][y][z]->r->stats.getAverageThroughput();////
			total_comms += ncomms;
		}
	}
    avg_throughput /= (double) total_comms;
    return avg_throughput;
}

unsigned int NoximGlobalStats::getReceivedPackets(){
    unsigned int n = 0;
	int x,y,z;
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++)
		n += noc->t[x][y][z]->r->stats.getReceivedPackets();
    return n;
}

unsigned int NoximGlobalStats::getReceivedFlits(){
    unsigned int n = 0;
    int x,y,z;
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++)
		n += noc->t[x][y][z]->r->stats.getReceivedFlits();
    return n;
}

double NoximGlobalStats::getThroughput()
{
    int total_cycles = 	NoximGlobalParams::simulation_time - NoximGlobalParams::stats_warm_up_time;
    unsigned int n = 0;
    unsigned int trf = 0;
    int x,y,z;
	unsigned int rf;
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++){
		rf = noc->t[x][y][z]->r->stats.getReceivedFlits();
		if (rf != 0)n++;
		trf += rf;
	}
    return (double) trf / (double) (total_cycles * n);
}

vector <vector < vector < unsigned long > > >NoximGlobalStats::getRoutedFlitsMtx(){
    vector < vector < vector < unsigned long > > > mtx;
    int x,y,z;
	mtx.resize( NoximGlobalParams::mesh_dim_z); 
	for ( z=0; z< NoximGlobalParams::mesh_dim_z; z++){
		mtx[z].resize( NoximGlobalParams::mesh_dim_x *  NoximGlobalParams::mesh_dim_y);
		for ( y=0; y<  NoximGlobalParams::mesh_dim_y; y++)
			mtx[z][y].resize( NoximGlobalParams::mesh_dim_x);////
	}
	for( z=0; z< NoximGlobalParams::mesh_dim_z; z++)////
	for( y=0; y< NoximGlobalParams::mesh_dim_y; y++)
	for( x=0; x< NoximGlobalParams::mesh_dim_x; x++)
		mtx[z][y][x] = noc->t[x][y][z]->r->getRoutedFlits();
	return mtx;
}
double NoximGlobalStats::getAverageWaitingTime(){
	int x,y,z;
	int wating = 0;
	int routed = 0;
	for( z=0; z< NoximGlobalParams::mesh_dim_z; z++)
	for( y=0; y< NoximGlobalParams::mesh_dim_y; y++)
	for( x=0; x< NoximGlobalParams::mesh_dim_x; x++){
		wating += noc->t[x][y][z]->r->getTotalWaitingTime();
		routed += noc->t[x][y][z]->r->getRoutedPackets   ();
	}
	
	return (double)wating/routed;
}

double NoximGlobalStats::getPower(){
    double power = 0.0;
	int x,y,z;
	for( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for( x=0; x<NoximGlobalParams::mesh_dim_x; x++)
		power += noc->t[x][y][z]->r->getPower();
    return power;
}

vector<double>  NoximGlobalStats::getLayerPower(){ 
	vector<double>   power;  
	int x,y,z;
	power.resize(NoximGlobalParams::mesh_dim_z); 
	for ( z=0; z<NoximGlobalParams::mesh_dim_z; z++)  
	for ( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for ( x=0; x<NoximGlobalParams::mesh_dim_x; x++)
		power[z] += noc->t[x][y][z]->r->getPower();  
	return power;  
}

void NoximGlobalStats::showStats(std::ostream & out, bool detailed)
{
	unsigned int z,x,y;
    out << "% Total received packets: "                  << getReceivedPackets    () << endl;
    out << "% Total received flits: "                    << getReceivedFlits      () << endl;
    out << "% Global average delay (cycles): "           << getAverageDelay       () << endl;
	/*out << "% Global average NI delay (cycles): "        << getNiAverageDelay     () << endl;
	out << "% Global average NW delay (cycles): "        << getNwAverageDelay     () << endl;
	out << "% Global Total Nw  avg delay (cycles): "     << getNwTotalAverageDelay() << endl;
	out << "% Global Total Ni  avg delay (cycles): "     << getNiTotalAverageDelay() << endl;
	out << "% Global Total Msg avg delay (cycles): "     << getMsgTotalAverageDelay() << endl;*/
    out << "% Global average throughput (flits/cycle): " <<	getAverageThroughput  () << endl;
    out << "% Throughput (flits/cycle/IP): "             << getThroughput         () << endl;
    out << "% Max delay (cycles): "                      << getMaxDelay           () << endl;
    out << "% Total energy (J): "                        << getPower              () << endl;
	out	<< "% Avg power (J/cycle): "                     <<	getPower()/NoximGlobalParams::simulation_time<< endl;
	out	<< "% Avg power per router (J/cycle): "          <<	getPower()/NoximGlobalParams::simulation_time/(NoximGlobalParams::mesh_dim_x*NoximGlobalParams::mesh_dim_y*NoximGlobalParams::mesh_dim_z) << endl;
	out << "% Avg waiting time in each buffer (cycles): "<< getAverageWaitingTime()<< endl; 
	out << "% Layer average delay (cycles): ";
	vector<double> delay_mtx = getLayerAverageDelay(); 	
	for ( z=0; z<delay_mtx.size(); z++)out << setw(6) <<delay_mtx[z]<<"  "; 
	out<<endl;
	out << "% Layer energy (J): " ;
    vector<double> power_mtx = getLayerPower(); 
	for ( z=0; z<power_mtx.size(); z++)	out << setw(6) <<power_mtx[z]<<"  ";
	out<<endl;
	out << "% Layer Routed flits: ";
	vector<unsigned int> flits_mtx = getLayerRoutedFlits(); 
	for ( z=0; z<flits_mtx.size(); z++)	out << setw(6) <<flits_mtx[z]<<"  ";
	out<<endl;
	for ( x = 0 ; x < DIRECTIONS + 1 ; x++){
		out << "% Layer Routed flits["<<x<<"]:";
		vector<unsigned int> flits_mtx = getLayerRoutedFlits(x);
		for ( z = 0 ; z< flits_mtx.size() ; z++ ) out << setw(6) <<flits_mtx[z]<<"  ";
		out<<endl;
	}
    getDelayHist();
	if (detailed) {
	out << endl << "detailed = [" << endl;
	for( z=0; z<NoximGlobalParams::mesh_dim_z; z++)
	for( y=0; y<NoximGlobalParams::mesh_dim_y; y++)
	for( x=0; x<NoximGlobalParams::mesh_dim_x; x++)
		noc->t[x][y][z]->r->stats.showStats( xyz2Id(x,y,z) ,out,true);
	out << "];" << endl;

	// show MaxDelay matrix
	vector <vector < vector < double > > > md_mtx = getMaxDelayMtx();

	out << endl << "max_delay = [" << endl;
	for( z=0; z<md_mtx.size(); z++)
	for( y=0; y<md_mtx[z].size(); y++){
		out << "   ";
		for ( x=0; x<md_mtx[z][y].size(); x++)
			out << setw(6) << md_mtx[z][y][x];
		out << endl;
	}
	out << "];" << endl;
	// show RoutedFlits matrix
	vector <vector < vector < unsigned long > > > rf_mtx = getRoutedFlitsMtx();
	out << endl << "routed_flits = [" << endl;
	for ( z=0; z<rf_mtx.size()    ; z++)
	for ( y=0; y<rf_mtx[z].size() ; y++){
		out << "   ";
		for ( x=0; x<rf_mtx[z][y].size(); x++)
			out << setw(10) << rf_mtx[z][y][x]<<endl;
	}
	out << "];" << endl;    }	
}
