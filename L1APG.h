#ifndef L1APG_H
#define L1APG_H

#include <vector>
#include <Eigen/Core>
#include<Eigen/SVD>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "Rect.h"
#include "Public.h"

using namespace std;
using namespace Eigen;

class L1APG
{
public:
	L1APG(const FloatRect&, const cv::Mat&, bool color, int numSample = 200); 
	// make sure that init is a one dimensional array with four elements

	void Reset(int);
	void IntializeTpl(const cv::Mat & frame);
	inline double * GetCurrentStatus(){ return m_curStatus;}
	inline bool IsInitialized() {return m_initialized;}
	void Track(const cv::Mat & frame);
	FloatRect GetBB();
	long int m_time;
private:
	void Whitening(double * feature, int size);
	void Whitening(MatrixXd & T);
	void L2Normalize(MatrixXd & T);
	void DrawSamples(const cv::Mat & frame);
	MatrixXd LSS(); // calculate the least squre solution for each sample using opencv
	VectorXd L2Error();
	VectorXd APG_BT(const MatrixXd &b,const MatrixXd & A,const MatrixXd & y,const MatrixXd & D,vector<double> & funVal,vector<double> & funVal2);
	VectorXd APG_Solver(const MatrixXd &b,const MatrixXd & A,const MatrixXd & y,const MatrixXd & D,vector<double> & funVal,vector<double> & funVal2);
	double L2Norm(VectorXd feat);
	double L1Norm(VectorXd feat);
	double LSum(VectorXd feat);
	double InnerProd(VectorXd feat_1,VectorXd feat_2);
	void Resample(vector<double> plh, const double * curStatus);
	double VectorAngle(VectorXd v1,VectorXd v2);
	void L2Normalize(double * feature, int size);
	void CheckRoi(cv::Rect &roi);

	int m_numSample;
	float m_affStd[3]; // [x(topleft) y(topleft) scale]
	int m_tplSize[2]; // template size [height width]
	float m_lambda[3]; //lambda 1, lambda 2 for a_T and a_I respectively, lambda 3 for the L2 norm parameter
	float m_angleThreshold;
	float m_tol; //used to stop the iteration of APG
	int m_maxIt; // the maximum number of iteration for APG
	int m_numTpl; // the number of normal templates
	float m_alpha; //this parameter is used in the calculation of the likelihood of particle filter
	double m_initStatus[4];//[x(topleft) y(topleft) width height]
	int m_occNf;
	float m_mu;// store the initial value of m_lambda[2];
	double m_level;// used to calculate the occlusion map
	long int m_seed; // used to initialize the random number generator
	double m_lip; // the default Lip constant
	
	bool m_initialized;
	MatrixXd m_temp; //  Temp = [A fixT];
	MatrixXd m_tempTran;
	MatrixXd m_A; // A = [T eye_dim_T];
	MatrixXd m_temp1; // Temp1 = [T,fixT]*pinv([T,fixT]);
	MatrixXd m_temp2; // Temp2 = [T,fixT];
	MatrixXd m_dict;// Temp'*Temp;
	bool m_isColor;
	int m_numChannel;
	MatrixXd m_samples;
	MatrixXd m_sampleFeature;
	double m_curStatus[4];
	int m_width;
	int m_height;
	cv::RNG m_rng;
};
#endif
