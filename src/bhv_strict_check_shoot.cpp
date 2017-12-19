// -*-c++-*-

#include <rcsc/action/kick_table.h>
#include <rcsc/game_time.h>
#include <rcsc/player/action_effector.h>
#include <rcsc/player/ball_object.h>
#include <rcsc/player/self_object.h>
#include <rcsc/player/world_model.h>
#include <rcsc/common/server_param.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <queue>
#include <iostream>
#include <fstream>



/*!
  \file bhv_strict_check_shoot.cpp
  \brief strict checked shoot behavior using ShootGenerator
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_strict_check_shoot.h"

#include "shoot_generator.h"

#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/common/logger.h>
#include <sstream>
using namespace rcsc;
using namespace std;
static int log_file_index = 0;

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_StrictCheckShoot::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! wm.self().isKickable() )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " not ball kickable!"
                  << std::endl;
        dlog.addText( Logger::ACTION,
                      __FILE__":  not kickable" );
        return false;
    }



    const ShootGenerator::Container & cont = ShootGenerator::instance().courses( wm );

    // update
    if ( cont.empty() )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__": no shoot course" );
        return false;
    }

    ShootGenerator::Container::const_iterator best_shoot
        = std::min_element( cont.begin(),
                            cont.end(),
                            ShootGenerator::ScoreCmp() );

    if ( best_shoot == cont.end() )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__": no best shoot" );
        return false;
    }

    // ML check
     //  if(!preCheckShoot(agent))
    	//   return false;


    // it is necessary to evaluate shoot courses

    agent->debugClient().addMessage( "Shoot" );
    agent->debugClient().setTarget( best_shoot->target_point_ );

    Vector2D one_step_vel
        = KickTable::calc_max_velocity( ( best_shoot->target_point_ - wm.ball().pos() ).th(),
                                        wm.self().kickRate(),
                                        wm.ball().vel() );
    double one_step_speed = one_step_vel.r();


    // ML check
           if(!preCheckShoot(agent,best_shoot->target_point_,one_step_speed))
        	   return false;

    dlog.addText( Logger::SHOOT,
                  __FILE__": shoot[%d] target=(%.2f, %.2f) first_speed=%f one_kick_max_speed=%f",
                  best_shoot->index_,
                  best_shoot->target_point_.x,
                  best_shoot->target_point_.y,
                  best_shoot->first_ball_speed_,
                  one_step_speed );

    if ( one_step_speed > best_shoot->first_ball_speed_ * 0.99 )
    {
        if ( Body_SmartKick( best_shoot->target_point_,
                             one_step_speed,
                             one_step_speed * 0.99 - 0.0001,
                             1 ).execute( agent ) )
        {

             agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
             agent->debugClient().addMessage( "Force1Step" );
             mlLog(agent,best_shoot->target_point_,one_step_speed);
             return true;
        }
    }

    if ( Body_SmartKick( best_shoot->target_point_,
                         best_shoot->first_ball_speed_,
                         best_shoot->first_ball_speed_ * 0.99,
                         3 ).execute( agent ) )
    {
        if ( ! doTurnNeckToShootPoint( agent, best_shoot->target_point_ ) )
        {
            agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
        }
        mlLog(agent,best_shoot->target_point_, best_shoot->first_ball_speed_);
        return true;
    }

    dlog.addText( Logger::SHOOT,
                  __FILE__": failed" );
    return false;
}
bool Bhv_StrictCheckShoot::preCheckShoot(rcsc::PlayerAgent * agent,const rcsc::Vector2D & shoot_point, const double vel) {


	//*
	// * this is the order of features
	// * time, ball.x, ball.y, target.x, target,y, ball.vel, goali_line_dist, goali_ball_dist, top 4 opp_line_dist, top 4 opp_ball_dist
	// *

	const WorldModel & wm = agent->world();
	double var1= wm.time().cycle();
	double var2= wm.ball().pos().x;
	double var3= wm.ball().pos().y;
	double var4= shoot_point.x ;
	double var5= shoot_point.y ;
	double var6= vel ;

	double opp_goali_ball_dist = 1000;
	double opp_goali_line_dist = 1000;
	double var7=opp_goali_line_dist;
	double var8=opp_goali_ball_dist;
	std::priority_queue<std::pair<double, int> > opp_line_dist;
	std::priority_queue<std::pair<double, int> > opp_ball_dist;
	opp_line_dist.push(std::pair<double, int>(-1000, 0));
	opp_ball_dist.push(std::pair<double, int>(-1000, 0));

	Line2D shoot_line(shoot_point, wm.ball().pos());
	const PlayerPtrCont::const_iterator end = wm.opponentsFromBall().end();
	int index=1;
	for (PlayerPtrCont::const_iterator p = wm.opponentsFromBall().begin();p != end; ++p) {
		double line_dist = shoot_line.dist((*p)->pos());
		double ball_dist = wm.ball().pos().dist((*p)->pos());
		if ((*p)->goalie()) {
			opp_goali_ball_dist = ball_dist;
			opp_goali_line_dist = line_dist;
			var7= opp_goali_line_dist;
			var8= opp_goali_ball_dist;
		} else if((*p)->pos().x+10 > wm.self().pos().x ){
			opp_line_dist.push(std::pair<double, int>(-1*line_dist, index));
			opp_ball_dist.push(std::pair<double, int>(-1*ball_dist, index));
			index++;
		}

	}

	// pick top 4 distances from line
	int k = 4; // number of indices we need

	double dist = opp_line_dist.top().first;
	double var9 = -1 * dist;
	opp_line_dist.pop();
	dist = opp_line_dist.top().first;
	double var10 = -1 * dist;
	opp_line_dist.pop();
	dist = opp_line_dist.top().first;
	double var11 = -1 * dist;
	opp_line_dist.pop();
	dist = opp_line_dist.top().first;
	double var12 = -1 * dist;
	opp_line_dist.pop();

	//pick top 4 distances from ball

	dist = opp_ball_dist.top().first;
	double var13 = -1 * dist;
	opp_ball_dist.pop();
	dist = opp_ball_dist.top().first;
	double var14 = -1 * dist;
	opp_ball_dist.pop();
	dist = opp_ball_dist.top().first;
	double var15 = -1 * dist;
	opp_ball_dist.pop();
	dist = opp_ball_dist.top().first;
	double var16 = -1 * dist;
	opp_ball_dist.pop();

	double result = var1 * (1.19807024e-04)
			+ var2 * (-2.38804668e-01) + var3 * (8.02485950e-03)
			+ var4 * ( 2.58258521e-01) + var5 * (6.37951951e-02)
			+ var6 * (8.79888791e-02) + var7 * (5.28266713e-01)
			+ var8 * (-5.18519277e-01) + var9 * (1.50666702e-01)
			+ var10 * (-5.53150551e-03) + var11 * (2.00101005e-02)
			+ var12 * (-6.47668961e-04) + var13 * (-7.22758973e-03)
			+ var14 * ( -1.54256700e-03) + var15 * ( -1.96752048e-02)
			+ var16 * (-8.63344514e-04);

	double prob=1/(1+std::exp(-1* result));
		if(prob>=.6)
			return true;
		else
			return false;

}

bool Bhv_StrictCheckShoot::preCheckShoot(rcsc::PlayerAgent * agent) {

	//*Implements Logistic Regression
	// * this is the order of features
	// * time, angle with goal, goalie ball distance,  top two closest opp in sector, top  two ratio of opp
	// *

	const WorldModel & wm = agent->world();

	const ServerParam & SP = ServerParam::i();
	/*
	 const Vector2D goal_l(-SP.pitchHalfLength(), -SP.goalHalfWidth());
	 const Vector2D goal_r(-SP.pitchHalfLength(), +SP.goalHalfWidth());
	 AngleDeg ball2post_angle_l = (goal_l - wm.ball().pos()).th();
	 AngleDeg ball2post_angle_r = (goal_r - wm.ball().pos()).th();
	 */
	const Vector2D goal = SP.theirTeamGoalPos();
	const AngleDeg goal_angle_from_ball = (goal - wm.ball().pos()).th();
	double var1 = wm.time().cycle();
	double var2 = goal_angle_from_ball.degree();

	const PlayerObject * opp_goalie = wm.getOpponentGoalie();
	double var3 = 0;
	if (opp_goalie)
		var3 = opp_goalie->distFromBall();
	else
		var3 = 10;

	int opp_counter = 0;
	Vector2D goal_l(SP.pitchHalfLength(), -SP.goalHalfWidth());
	Vector2D goal_r(SP.pitchHalfLength(), +SP.goalHalfWidth());
	Line2D line_l(goal_l, wm.self().pos());
	Line2D line_r(goal_r, wm.self().pos());
	double opp_goali_ball_dist = 20;
	std::priority_queue<std::pair<double, int> > opp_dist_list;
	opp_dist_list.push(std::pair<double, int>(-10, 0));
	std::priority_queue<std::pair<double, int> > opp_dist_ratio_list;
	opp_dist_ratio_list.push(std::pair<double, int>(0, 0));
	int index = 1;
	const Triangle2D triangle(goal_l, goal_r, wm.self().pos());
	const PlayerPtrCont::const_iterator end = wm.opponentsFromBall().end();
	for (PlayerPtrCont::const_iterator it = wm.opponentsFromBall().begin();
			it != end; ++it) {
		if ((*it)->posCount() > 10 || (*it)->isGhost()) {
			continue;
		}
		if ((*it)->goalie()) {
			continue;
		}
		if (triangle.contains((*it)->pos())) {
			opp_counter++;
			double opp_dist = wm.self().pos().dist((*it)->pos());
			opp_dist_list.push(std::pair<double, int>(-1 * opp_dist, index));
			double dist1 = line_l.dist((*it)->pos());
			double dist2 = line_r.dist((*it)->pos());
			double ratio = 1;
			if (dist1 > dist2)
				ratio = dist2 / dist1;
			else
				ratio = dist1 / dist2;
			opp_dist_ratio_list.push(std::pair<double, int>(ratio, index));
			index++;
		}
	}

	double var4 = opp_counter;
	double var5 = opp_dist_list.top().first;
	opp_dist_list.pop();
	double var6 = opp_dist_list.top().first;
	opp_dist_list.pop();

	double var7 = opp_dist_ratio_list.top().first;
	opp_dist_ratio_list.pop();
	double var8 = opp_dist_ratio_list.top().first;
	opp_dist_ratio_list.pop();

	double result= -0.28468837+ var1 * 0.06185962 + var2 * (-0.35727921) + var3 * (-0.35821888) + var4 * 0.24679721 +
	var5 * 0.19454455 + var6 * 0.16976452 + var7 * (-0.18586534) + var8 * (-0.16252184);

	double prob=1/(1+std::exp(-1* result));
	if(prob>=.6)
		return true;
	else
		return false;
}

// ml log 8 feature
void Bhv_StrictCheckShoot::mlLog(rcsc::PlayerAgent * agent) {
	//*
	// * this is the order of features
	// * time, angle with goal, goalie ball distance,  top two closest opp in sector, top  two ratio of opp
	// *

	const WorldModel & wm = agent->world();
	// build log index

	ifstream filename;
	filename.open("/home/navid/robo/log/log_index.txt");
	if (!filename)
		cerr << "Unable to open file datafile.txt";
	filename >> log_file_index;
	filename.close();


	ofstream myfile;
	std::stringstream log_file_name_tmp;
	log_file_name_tmp << "/home/navid/robo/log/shoots" << log_file_index << ".txt";
	std::string log_file_name = log_file_name_tmp.str();
	myfile.open(log_file_name.c_str(), ios::app);

	const ServerParam & SP = ServerParam::i();

	// const Vector2D goal_l(-SP.pitchHalfLength(), -SP.goalHalfWidth());
	// const Vector2D goal_r(-SP.pitchHalfLength(), +SP.goalHalfWidth());
	// AngleDeg ball2post_angle_l = (goal_l - wm.ball().pos()).th();
	// AngleDeg ball2post_angle_r = (goal_r - wm.ball().pos()).th();

	const Vector2D goal = SP.theirTeamGoalPos();
	const AngleDeg goal_angle_from_ball = (goal - wm.ball().pos()).th();
	myfile << wm.time().cycle() << "\t" << goal_angle_from_ball << "\t";

	const PlayerObject * opp_goalie = wm.getOpponentGoalie();
	if (opp_goalie)
		myfile << opp_goalie->distFromBall() << "\t";
	else
		myfile << 10 << "\t";

	int opp_counter = 0;
	Vector2D goal_l(SP.pitchHalfLength(), -SP.goalHalfWidth());
	Vector2D goal_r(SP.pitchHalfLength(), +SP.goalHalfWidth());
	Line2D line_l(goal_l, wm.self().pos());
	Line2D line_r(goal_r, wm.self().pos());
	double opp_goali_ball_dist = 20;
	std::priority_queue<std::pair<double, int> > opp_dist_list;
	opp_dist_list.push(std::pair<double, int>(-10, 0));
	std::priority_queue<std::pair<double, int> > opp_dist_ratio_list;
	opp_dist_ratio_list.push(std::pair<double, int>(0, 0));
	int index = 1;
	const Triangle2D triangle(goal_l, goal_r, wm.self().pos());
	const PlayerPtrCont::const_iterator end = wm.opponentsFromBall().end();
	for (PlayerPtrCont::const_iterator it = wm.opponentsFromBall().begin();
			it != end; ++it) {
		if ((*it)->posCount() > 10 || (*it)->isGhost()) {
			continue;
		}
		if ((*it)->goalie()) {
			continue;
		}
		if (triangle.contains((*it)->pos())) {
			opp_counter++;
			double opp_dist = wm.self().pos().dist((*it)->pos());
			opp_dist_list.push(std::pair<double, int>(-1 * opp_dist, index));
			double dist1 = line_l.dist((*it)->pos());
			double dist2 = line_r.dist((*it)->pos());
			double ratio = 1;
			if (dist1 > dist2)
				ratio = dist2 / dist1;
			else
				ratio = dist1 / dist2;
			opp_dist_ratio_list.push(std::pair<double, int>(ratio, index));
			index++;
		}
	}

	myfile << opp_counter << "\t";

	// pick top 4 distances from line
	int k = 2; // number of indices we need
	for (int i = 0; i < k; ++i) {
		double dist = opp_dist_list.top().first;
		myfile << -1 * dist << "\t";
		opp_dist_list.pop();
	}
	for (int i = 0; i < k; ++i) {
		double ratio = opp_dist_ratio_list.top().first;
		myfile << ratio << "\t";
		opp_dist_ratio_list.pop();
	}

	myfile << std::endl;
	myfile.close();

}


//this method log 16 features
void Bhv_StrictCheckShoot::mlLog(rcsc::PlayerAgent * agent,const rcsc::Vector2D & shoot_point, const double vel) {

	//*
	// * this is the order of features
	// * time, ball.x, ball.y, target.x, target,y, ball.vel, goali_line_dist, goali_ball_dist, top 4 opp_line_dist, top 4 opp_ball_dist
	// *

	ifstream filename;
	filename.open("/home/navid/robo/log/log_index.txt");
	if (!filename)
		cerr << "Unable to open file datafile.txt";
	filename >> log_file_index;
	filename.close();

	ofstream myfile;
	std::stringstream log_file_name_tmp;
	log_file_name_tmp << "/home/navid/robo/log/shoots" << log_file_index << ".txt";
	std::string log_file_name = log_file_name_tmp.str();
	myfile.open(log_file_name.c_str(), ios::app);

	const WorldModel & wm = agent->world();
	myfile << wm.time().cycle()<<"\t"<< wm.ball().pos().x<<"\t"<< wm.ball().pos().y<<"\t" << shoot_point.x << "\t" << shoot_point.y << "\t"<< vel <<"\t";

	double opp_goali_ball_dist = 1000;
	double opp_goali_line_dist = 1000;
	std::priority_queue<std::pair<double, int> > opp_line_dist;
	std::priority_queue<std::pair<double, int> > opp_ball_dist;
	opp_line_dist.push(std::pair<double, int>(-1000, 0));
	opp_ball_dist.push(std::pair<double, int>(-1000, 0));

	Line2D shoot_line(shoot_point, wm.ball().pos());
	const PlayerPtrCont::const_iterator end = wm.opponentsFromBall().end();
	int index=1;
	for (PlayerPtrCont::const_iterator p = wm.opponentsFromBall().begin();p != end; ++p) {
		double line_dist = shoot_line.dist((*p)->pos());
		double ball_dist = wm.ball().pos().dist((*p)->pos());
		if ((*p)->goalie()) {
			opp_goali_ball_dist = ball_dist;
			opp_goali_line_dist = line_dist;
			myfile << opp_goali_line_dist<<"\t";
			myfile << opp_goali_ball_dist<<"\t";
		} else if((*p)->pos().x+10 > wm.self().pos().x ){
			opp_line_dist.push(std::pair<double, int>(-1*line_dist, index));
			opp_ball_dist.push(std::pair<double, int>(-1*ball_dist, index));
			index++;
		}

	}

	// pick top 4 distances from line
	int k = 4; // number of indices we need
	for (int i = 0; i < k; ++i) {
		double dist = opp_line_dist.top().first;
		myfile << -1*dist <<"\t";
		opp_line_dist.pop();
	}
	//pick top 4 distances from ball
	for (int i = 0; i < k; ++i) {
			double dist = opp_ball_dist.top().first;
			myfile << -1*dist <<"\t";
			opp_ball_dist.pop();
	}

	myfile << std::endl;;
	myfile.close();
}



/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_StrictCheckShoot::doTurnNeckToShootPoint( PlayerAgent * agent,
                                              const Vector2D & shoot_point )
{
    const double angle_buf = 10.0; // Magic Number

    if ( ! agent->effector().queuedNextCanSeeWithTurnNeck( shoot_point, angle_buf ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": cannot look the shoot point(%.2f %.2f)",
                      shoot_point.x,
                      shoot_point.y );
        return false;
    }

#if 0
    const WorldModel & wm = agent->world();
    if ( wm.seeTime() == wm.time() )
    {
        double current_width = wm.self().viewWidth().width();
        AngleDeg target_angle = ( shoot_point - wm.self().pos() ).th();
        double angle_diff = ( target_angle - wm.self().face() ).abs();

        if ( angle_diff < current_width*0.5 - angle_buf )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": already seen. width=%.1f, diff=%.1f. shoot point(%.2f %.2f)",
                          current_width,
                          angle_diff,
                          shoot_point.x,
                          shoot_point.y );
            return false;
        }
    }
#endif

    dlog.addText( Logger::TEAM,
                  __FILE__": turn_neck to the shoot point(%.2f %.2f)",
                  shoot_point.x,
                  shoot_point.y );
    agent->debugClient().addMessage( "Shoot:NeckToTarget" );

    agent->setNeckAction( new Neck_TurnToPoint( shoot_point ) );

    return true;
}
