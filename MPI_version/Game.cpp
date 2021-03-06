#include "Game.hpp"
#include "Set_System.hpp"
#include "SaaS_Problem.hpp"
#include "SaaS.hpp"
#include "IaaS.hpp"
#include "IaaS_Problem.hpp"
#include "application.hpp"
#include "WS.hpp"
#include "Set_System.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <ios>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <mpi.h>


Game::Game( unsigned seed , const std::string& config_file_name )
{
  // first create a Set_System object, the use its function set, to set the problem
  Set_System system( seed, config_file_name );
  auto problem = system.set();

  // now get the IaaS and the SaaSs in order to create the parameters files
  iaas = std::make_shared < IaaS_Problem >( problem.first );
  for( auto & saas_problem : problem.second )
  {
    SaaSs.push_back(std::make_shared< SaaS_Problem >( saas_problem ));
  }

  //create the parameters files
  create_files();
}


Game::Game( unsigned seed, const std::string& config_file_name,  const std::string& global_parameters, const std::string& SaaSs_parameters, const std::string& WSs_parameters )
{
  Set_System system( seed, config_file_name );
  auto problem = system.set_from_files( global_parameters, SaaSs_parameters, WSs_parameters );
  iaas = std::make_shared < IaaS_Problem >( problem.first );
  for( auto & saas_problem : problem.second )
  {
    SaaSs.push_back(std::make_shared< SaaS_Problem >( saas_problem ));
  }
}


std::vector<std::shared_ptr<SaaS_Problem>> Game::get_SaaSs( void )
{
  return SaaSs;
}

std::shared_ptr< IaaS_Problem > Game::get_iaas( void )
{
  return iaas;
}


/*
  this function compute the total number of VMs used in the problem and also the number of VMs for each SaaS
  it returns a pair which has as first component the total number of VMs needed by the system of SaaSs, nad as second component
  a vector which contains the number of VMs for each SaaS
*/
double Game::compute_total_n( void )
{
  double total_n = 0; // total number of VMs requested
  std::vector< double > total_n_j; // number of VMs requested by SaaS j
  for(unsigned i = 0; i < SaaSs.size(); i++)
  {
    for(auto & app : SaaSs[i] -> get_SaaS() -> get_applications() )
      total_n += ( SaaSs[i] -> get_SaaS() -> get_on_flat(app) + SaaSs[i] -> get_SaaS() -> get_on_demand(app) );
  }
  //std::cout << "total_n= "<<total_n << '\n';
  return  total_n;
}

/*
  this function checks if all the SaaSs have been satisfied in terms of on_spot VMs
*/
bool Game::check_system( void )
{
  bool target = true;
  for( auto saas_problem : SaaSs )
    target *= saas_problem -> check();
  return target;
}

/*
  this function checks if some cost_threshold's change is needed in order to try to get the desired_on_spot
  it returns a pair, the first element is a bool which indicates if all the SaaSs are satisfied or not, and the second element is a vector
  of bool which indicates for each SaaS if it is satisfied or not
*/
std::pair< bool, std::vector<bool> > Game::check_changes( void )
{
  int target = 0;
  std::vector< bool > check_j;
  for( unsigned i = 0; i < SaaSs.size(); i++ )
  {
    bool target_temp = SaaSs[i] -> check();
    double current_price = SaaSs[i] -> get_cost_threshold();
    if( ! target_temp ) // if the i-th SaaS is not satisfied then augment its cost_threshold
    {
      SaaSs[i] -> set_cost_threshold();
    }
    if( current_price != SaaSs[i] -> get_cost_threshold() ) // if the price changed it means that the SaaS is not satisfied and
    {                                                       // it does not reach the maximum threshold yet
      target += 1;
      check_j.push_back( true );
    }
    else
    {
      check_j.push_back( false );
    }
  }
  return std::make_pair( target > 0, check_j);
}


void Game::print( std::ofstream& ofs1 )
{
  iaas -> print( ofs1 );
  for( unsigned i = 0; i < SaaSs.size(); i++ )
  {
    ofs1 << '\n' << "SAAS_PROBLEM "<< i <<'\n';
    SaaSs[i] -> print( ofs1 );
  }
}

/*
  this function at first initializes the system randomly then it starts to create the files
  in which it wites the parameters
*/
void Game::create_files( void )
{
  // create the parameters files needed
  std::fstream global_file;
  std::fstream SaaSs_file;
  std::fstream WSs_file;

  // open the files, and reset them
  global_file.open("global_parameters.csv", std::ios::out | std::ios::trunc );
  SaaSs_file.open("SaaSs_parameters.csv", std::ios::out | std::ios::trunc );
  WSs_file.open("WSs_parameters.csv", std::ios::out | std::ios::trunc );

  // call to the functions needed to write the parameters
  create_global_file( global_file );
  create_SaaSs_file( SaaSs_file );
  create_WSs_file( WSs_file );

}

void Game::create_global_file( std::fstream & global_file )
{

  // get the global parameters from one SaaS
  unsigned T = SaaSs[0] -> get_T();
  unsigned N = SaaSs[0] -> get_N();
  unsigned gamma = SaaSs[0] -> get_gamma();
  double rho = SaaSs[0] -> get_rho();
  double delta = SaaSs[0] -> get_delta();
  double mu_dev = SaaSs[0] -> get_mu_dev();
  double D_dev = SaaSs[0] -> get_D_dev();
  double omega = iaas -> get_omega();

  global_file << "T , N , gamma , rho , delta , mu_dev , D_dev , omega\n";

  global_file << T << "," << N << "," << gamma << ","<< rho << ","<< delta << "," << mu_dev << ","<< D_dev << ","<< omega <<'\n';

}

void Game::create_SaaSs_file( std::fstream & SaaSs_file )
{
  // initialize the parameters
  double q_j = .0;
  double eta_j = .0;
  unsigned R_j = 0;
  unsigned SaaS_index = 0;

  //create the columns names
  SaaSs_file << "q_j , eta_j , R_j , SaaS_index\n";


  // for each SaaS we write the parameters in the file
  for( auto & saas : SaaSs)
  {
    q_j = saas -> get_q_j();
    eta_j = saas -> get_eta_j();
    R_j = saas -> get_R_j();

    SaaSs_file << q_j << "," << eta_j << "," << R_j << ","<< SaaS_index <<'\n';

    // we write in the file also the SaaS_index, in order to recognize the parameters
    SaaS_index++;
  }
}


// the same of create_SaaSs_file but here we have the parameters for each WS
void Game::create_WSs_file( std::fstream & WSs_file )
{
  double LAMBDA_a_w = .0;
  double lambda_a_w = .0;
  double D_a_w = .0;
  double mu_a_w = .0;
  double R_bar_a_w = .0;
  double nu_a_w = .0;

  // we put an index in order to understand the WS, the app and the SaaS of a given line of parameters
  unsigned SaaS_index = 0;
  unsigned app_index = 0;
  unsigned WS_index = 0;

  WSs_file <<  "LAMBDA_a_w , lambda_a_w , D_a_w , mu_a_w , R_bar_a_w , nu_a_w, SaaS_index , app_index , WS_index\n";

  for( auto & saas : SaaSs )
  {

    auto apps = saas -> get_SaaS() -> get_applications();

    for( auto & app : apps )
    {
      auto WSs = app.get_web_services();

      for( auto & WS : WSs)
      {

        LAMBDA_a_w = WS.get_LAMBDA_a_w();
        lambda_a_w = WS.get_lambda_a_w();
        D_a_w = WS.get_D_a_w();
        mu_a_w = WS.get_mu_a_w();
        R_bar_a_w = WS.get_R_bar_a_w();
        nu_a_w = WS.get_nu_a_w();

        WSs_file << LAMBDA_a_w << "," << lambda_a_w << "," << D_a_w << "," << mu_a_w << "," << R_bar_a_w << ","
                 << nu_a_w << "," << SaaS_index << "," << app_index << "," << WS_index <<'\n';

        WS_index++;

      }

    WS_index = 0;
    app_index++;

    }
    app_index = 0;
    SaaS_index++;

  }
}

void Game::reset_SaaSs_given_on_spot( void )     // this function reset the given_on_spot for each SaaS before of each call of solve_greedy()
{
  for( auto & ss : SaaSs )
    ss -> reset_given_on_spot();
}


// solve function, this function solve the optimization problem
void Game::solve( void )
{
  int  rank, size;
  MPI_Comm_size (MPI_COMM_WORLD, &size); //storing rank and size
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);

  auto saass = get_SaaSs(); // we take a vector of shared_ptr to the SaaSs
  auto iaas = get_iaas(); // we take a shared_ptr to the IaaS

  for( unsigned i = rank; i < saass.size(); i += size ) // each rank solves its SaaS_Problems following a round robin cycle
  {
    saass[i] -> solve();
    saass[i] -> rounding();

    auto s = saass[i] -> get_SaaS(); // now we have to send the results to the rank0 in order to make it able to solve the IaaS_Problem
    n_app = s -> get_size();
    double on_flat = .0;
    double on_demand = .0;
    double desired_on_spot = .0;

    if( rank != 0)  //if the ranks is not rank0 it sends the result to rank0 (number of on_flat, on_demand and desired_on_spot VMs)
    {
      for ( unsigned j = 0; j < n_app; j++ ) //cycle over the app of the SaaS
      {
          on_flat = s -> get_on_flat(j); //get the results
          on_demand = s -> get_on_demand(j);
          desired_on_spot = s -> get_desired_on_spot(j);
          MPI_Send(&on_flat, 1, MPI_DOUBLE, 0,0, MPI_COMM_WORLD); //send the results
          MPI_Send(&on_demand, 1, MPI_DOUBLE, 0,0, MPI_COMM_WORLD);
          MPI_Send(&desired_on_spot, 1, MPI_DOUBLE, 0,0, MPI_COMM_WORLD);
      }
    }
    else //if the rank is the rank0
    {
      for (int r=1; r<size; r++) // we have to receive the results from the other ranks
      {
        if( r+i < saass.size() ) // if we're still waiting other results
        {
          for ( unsigned j = 0; j < n_app; j++ ) //cycleover the apps
          {
            MPI_Recv(&on_flat, 1, MPI_DOUBLE,r,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE); //receive the results
            MPI_Recv(&on_demand, 1, MPI_DOUBLE,r,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&desired_on_spot, 1, MPI_DOUBLE,r,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            saass[r+i]->get_SaaS()->set_on_flat(j,on_flat); // store the results
            saass[r+i]->get_SaaS()->set_on_demand(j,on_demand);
            saass[r+i]->get_SaaS()->set_desired_on_spot(j,desired_on_spot);
          }
        }
      }
    }
  }

  if(rank == 0)
  {
    // now rank0 has to solvethe IaaS problem, which is the sequential part of the code so we compute also the time required for the scalability analysis
    clock_t start,end;
    start=clock();

    //here we control that there are enough VMs at the IaaS in order to solve the problem
    if ( compute_total_n() > saass[0] -> get_N() )
    {
      std::cerr << "Insufficient resources" << '\n';
      return;
    }

    iaas -> solve_greedy(); // we solve the IaaS problem, here each SaaS has the on_spot coputed in the field " given_on_spot"

    //while some SaaSs are not satisfied at all, and those SaaSs can increase their offert, then recompute their cost_threshold
    //Then solve again the IaaS_Problem in order to update the number of on_spot for each SaaS
    bool target1 = !check_system(); // equal to 1 if some SaaS is not satisfied, 0 otherwise
    auto temp = check_changes();
    bool target2 = temp.first; // equal to 1 if some cost_threshold is changed, 0 otherwise
    std::vector<bool> cost_changes_target = temp.second; // vector that indicates which SaaS changed its cost_threshold

    while( target1 && target2 ) // while there is still some SaaS unsatisfied and those SaaSs still increase their offer
    {
      this -> reset_SaaSs_given_on_spot(); // reset the number of given_on_spot
      iaas -> solve_greedy(); //solve again the iaas problem
      target1 = !check_system(); //we check again
      temp = check_changes();
      target2 = temp.first;
      cost_changes_target = temp.second;
    }
    end=clock();
    iaas_time=((double)(end-start))/CLOCKS_PER_SEC; //this is the time required
  }

  double given_on_spot = .0;
  if(rank == 0) // here the rank0 sends the result of the iaas problem to the other ranks (numbero of given_on_spot VMs)
  {
    for (int r=1; r<size; r++)
    {
      for( unsigned i = r; i < saass.size(); i += size)
      {
        for ( unsigned j = 0; j < n_app; j++ )
        {
          given_on_spot = saass[i]->get_SaaS()->get_given_on_spot(j);
          MPI_Send(&given_on_spot,1,MPI_DOUBLE,r,0,MPI_COMM_WORLD);
        }
      }
    }
  }
  else
  {
    for( unsigned i = rank; i < saass.size(); i += size)
    {
      for ( unsigned j = 0; j < n_app; j++ )
      {
        MPI_Recv(&given_on_spot, 1, MPI_DOUBLE,0,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        saass[i] -> get_SaaS() ->set_given_on_spot(j, given_on_spot);
      }
    }
  }

  //Agein each rank solve the final SaaS problem for tis SaaSs and send the result to the rank0 which print the overall results in the file results.txt
  // at this step, we reached the local_optimum for the on_spot VMs
  for( unsigned i = rank; i < saass.size(); i += size  )
  {
    saass[i] -> update(); //hence we update the SaaS data storing the given_on_spot in the on_spot vector
    saass[i] -> solve();  // then we solve again the SaaS problem, in order to compute the right number of on_flat and on_demand VMs, taking into account the final number of on_spot VMS
    saass[i] -> reset_desired(); // here we reset the desired_on_spot which are meaningless at this step ( we have already computed the final number of on_spot)
    saass[i] -> rounding(); // now round the problem in order to get the integer values
    auto s = saass[i] -> get_SaaS();
    double on_flat = .0;
    double on_demand = .0;
    double on_spot = .0;

    if( rank != 0)
    {
      for ( unsigned j = 0; j < n_app; j++ )
      {
          on_flat = s -> get_on_flat(j);
          on_demand = s -> get_on_demand(j);
          on_spot = s -> get_on_spot(j);
          MPI_Send(&on_flat, 1, MPI_DOUBLE, 0,0, MPI_COMM_WORLD);
          MPI_Send(&on_demand, 1, MPI_DOUBLE, 0,0, MPI_COMM_WORLD);
          MPI_Send(&on_spot, 1, MPI_DOUBLE, 0,0, MPI_COMM_WORLD);
      }
    }
    else
    {
      for (int r=1; r<size; r++)
      {
        if(r+i < saass.size())
        {
          for ( unsigned j = 0; j < n_app; j++ )
          {
            MPI_Recv(&on_flat, 1, MPI_DOUBLE,r,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&on_demand, 1, MPI_DOUBLE,r,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&on_spot, 1, MPI_DOUBLE,r,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            saass[r+i]->get_SaaS()->set_on_flat(j,on_flat);
            saass[r+i]->get_SaaS()->set_on_demand(j,on_demand);
            saass[r+i]->get_SaaS()->set_on_spot(j,on_spot);
          }
        }
      }
    }
  }
  if( rank == 0)
  {
    std::cout << "The time required to solve the IaaS_Problem is:"<< iaas_time << '\n';
    // then print the results in results.txt
    std::string output_file1("results.txt");
    std::ofstream ofs1;
    ofs1.open( output_file1, std::ios::out | std::ios::trunc ); // i open the file and delete everything

    print( ofs1 );
  }
}
