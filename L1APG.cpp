#include "L1APG.h"
#include "ImageRep.h"
#include <time.h>
#include <Eigen/Dense>
#include <Eigen/SVD> 
#include <iostream>
#include <cmath>
#include <fstream>
#include <cstdio>

using namespace cv;
using namespace Eigen;
using namespace std;

//#define IS_DEBUG 

L1APG::L1APG(const FloatRect& bb, const Mat& img, bool color, int numSample) {
	
	/*
	if(init.size()!=4){
		abortError(__LINE__,__FILE__,"The size of init has to be four.");
	}
	*/
	m_initStatus[0] = m_curStatus[0] = bb.XMin();
	m_initStatus[1] = m_curStatus[1] = bb.YMin();
	m_initStatus[2] = m_curStatus[2] = bb.Width();
	m_initStatus[3] = m_curStatus[3] = bb.Height();

	Reset(numSample);

	m_width = img.cols;
	m_height = img.rows;
	m_isColor = color;
	if(m_isColor)
		m_numChannel = 3;

	m_time = 0;
}

void L1APG::Reset(int numSample){
	m_numSample = numSample;
	m_affStd[0] = 10;
	m_affStd[1] = 10;
	m_affStd[2] = 0.005;
	m_tplSize[0] = 16;
	m_tplSize[1] = 16;
	m_lambda[0] = 0.01;
	m_lambda[1] = 0.001;
	m_lambda[2] = 1;
	m_mu = m_lambda[2];
	m_angleThreshold = 40;
	m_tol = 1e-4;
	m_maxIt = 5;
	m_lip = 8;
	m_numTpl = 10;
	m_alpha = 50;
	m_level = 0.03;
	m_seed = 10;
	m_initialized = false;
	m_isColor = false;
	m_numChannel = 1;
}

void L1APG::CheckRoi(cv::Rect &roi){

	if(roi.width<=1){
		roi.width = 1;
	}

	if(roi.height<=1){
		roi.height = 1;
	}

	if(roi.x<0){
		roi.x=0;
	}
	if(roi.x>=m_width-2){
		roi.x=m_width-3;
	}
	if(roi.x+roi.width>=m_width){
		roi.width = m_width-1-roi.x;
	}

	if(roi.y<0){
		roi.y=0;
	}
	if(roi.y>=m_height-2){
		roi.y = m_height-3;
	}
	if(roi.y+roi.height>=m_height){
		roi.height = m_height-1-roi.y;
	}

}

void L1APG::IntializeTpl(const cv::Mat & frame){
	
	m_rng.state=m_seed;
	ImageRep imgRep(frame,m_isColor);
	double imageStatus[10][4];
	imageStatus[0][0] = m_initStatus[0];
	imageStatus[0][1] = m_initStatus[1];
	imageStatus[0][2] = m_initStatus[2];
	imageStatus[0][3] = m_initStatus[3];

	imageStatus[1][0] = m_initStatus[0]-1;
	imageStatus[1][1] = m_initStatus[1]-1;

	imageStatus[2][0] = m_initStatus[0]-1;
	imageStatus[2][1] = m_initStatus[1];

	imageStatus[3][0] = m_initStatus[0]-1;
	imageStatus[3][1] = m_initStatus[1]+1;

	imageStatus[4][0] = m_initStatus[0];
	imageStatus[4][1] = m_initStatus[1]-1;

	imageStatus[5][0] = m_initStatus[0];
	imageStatus[5][1] = m_initStatus[1]+1;

	imageStatus[6][0] = m_initStatus[0]+1;
	imageStatus[6][1] = m_initStatus[1]-1;

	imageStatus[7][0] = m_initStatus[0]+1;
	imageStatus[7][1] = m_initStatus[1];

	imageStatus[8][0] = m_initStatus[0]+1;
	imageStatus[8][1] = m_initStatus[1]+1;

	imageStatus[9][0] = m_initStatus[0]+2;
	imageStatus[9][1] = m_initStatus[1];
	for(int i=0;i<10;i++){
		imageStatus[i][2] = m_initStatus[2];
		imageStatus[i][3] = m_initStatus[3];
	}

	/*imageStatus[0][0] = m_initStatus[0];
	imageStatus[0][1] = m_initStatus[1];
	imageStatus[0][2] = m_initStatus[2];
	imageStatus[0][3] = m_initStatus[3];
	for(int i=1;i<10;i++){
		double r1,r2;
		r1 = m_rng.gaussian(1.0);
		r2 = m_rng.gaussian(1.0);
		imageStatus[i][0] =  m_initStatus[0]+r1;
		imageStatus[i][1] =  m_initStatus[1]+r2;
		imageStatus[i][2] =  m_initStatus[2];
		imageStatus[i][3] =  m_initStatus[3];
	}*/
	
	MatrixXd T;
	if(!m_isColor){
		//MatrixXd T(m_tplSize[1]*m_tplSize[2],10);
		T.resize(m_tplSize[0]*m_tplSize[1],10);
		for(int i=0;i<10;i++){
			cv::Rect roi(imageStatus[i][0], imageStatus[i][1], imageStatus[i][2], imageStatus[i][3]);
			CheckRoi(roi);
			Mat img_roi = imgRep.GetImage(0)(roi);
			Mat im(m_tplSize[0], m_tplSize[1], CV_8UC1);
			cv::resize(img_roi, im, im.size());
//			imwrite("1.jpg",im);
			//double * feature = new double[im.rows*im.cols];
			int inx = 0;
			for(int j=0;j<im.rows;j++){
				for(int k=0;k<im.cols;k++){
					//feature[inx++] = (int)im.at<uchar>(j,k);
					T(inx++,i) = (int)im.at<uchar>(j,k);
				}
			}
			//Whitening(feature,im.rows*im.cols);
			//L2Normalize(feature,im.rows*im.cols);
			//for(int j=0;j<im.rows*im.cols;j++){
			//	T(j,i) = feature[j];
			//}
			//delete[] feature;
		}
	}
	else{
		
		T.resize(m_tplSize[0]*m_tplSize[1]*m_numChannel,10);
		
		int ginx=0;
		for(int nc=0;nc<m_numChannel;nc++){
			for(int i=0;i<10;i++){
				cv::Rect roi(imageStatus[i][0], imageStatus[i][1], imageStatus[i][2], imageStatus[i][3]);
				CheckRoi(roi);
				Mat img_roi = imgRep.GetOppoImage(nc)(roi);
				Mat im(m_tplSize[0], m_tplSize[1], CV_32FC1);
				cv::resize(img_roi, im, im.size());
				//double * feature = new double[im.rows*im.cols];
				int inx = 0;
				for(int j=0;j<im.rows;j++){
					for(int k=0;k<im.cols;k++){
						//feature[inx++] = im.at<float>(j,k);
						T(ginx+inx,i)=im.at<float>(j,k);
						++inx;
					}
				}
				//Whitening(feature,im.rows*im.cols);
				//L2Normalize(feature,im.rows*im.cols);
				//for(int j=0;j<im.rows*im.cols;j++){
				//	T(j+ginx,i) = feature[j];
				//}
				//delete[] feature;
			}
			ginx+=m_tplSize[0]*m_tplSize[1];
		}
	}
	Whitening(T);
	L2Normalize(T);
	m_temp2.resize(m_tplSize[0]*m_tplSize[1]*m_numChannel,m_numTpl+1);
	m_temp.resize(m_tplSize[0]*m_tplSize[1]*m_numChannel,m_tplSize[0]*m_tplSize[1]*m_numChannel+m_numTpl+1);
	for(int i=0;i<m_numTpl;i++){
		for(int j=0;j<m_tplSize[0]*m_tplSize[1]*m_numChannel;j++){
			m_temp(j,i)=T(j,i);
			m_temp2(j,i)=T(j,i);
		}
	}
	for(int i=0;i<m_tplSize[0]*m_tplSize[1]*m_numChannel;i++){
		for(int j=m_numTpl;j<m_tplSize[0]*m_tplSize[1]*m_numChannel+m_numTpl;j++){
			if(i==j-m_numTpl){
				m_temp(i,j)=1;
			}
			else{
				m_temp(i,j)=0;
			}
		}
	}
	for(int i=0;i<m_tplSize[0]*m_tplSize[1]*m_numChannel;i++){
		int k1,k2;
		k1 = m_tplSize[0]*m_tplSize[1]*m_numChannel+m_numTpl+1-1;
		k2 = m_numTpl+1-1;
		m_temp(i,k1) = T(i,0)/m_numTpl;
		m_temp2(i,k2) = T(i,0)/m_numTpl; 
	}
	m_curStatus[0]=m_initStatus[0];
	m_curStatus[1]=m_initStatus[1];
	m_curStatus[2]=m_initStatus[2];
	m_curStatus[3]=m_initStatus[3];
	
	m_samples.resize(m_numSample,4);
	for(int i=0;i<m_numSample;i++){
		m_samples(i,0) = m_initStatus[0];
		m_samples(i,1) = m_initStatus[1];
		m_samples(i,2) = m_initStatus[2];
		m_samples(i,3) = m_initStatus[3];
	}
	m_tempTran = m_temp.transpose();
	m_sampleFeature.resize(m_tplSize[0]*m_tplSize[1]*m_numChannel,m_numSample);
	m_dict = m_temp.transpose()*m_temp;
	m_occNf = 0;
	
	m_initialized = true;
	
#ifdef IS_DEBUG
	FILE * file = fopen("templates_all.txt","w+");
	for(int i=0;i<m_tplSize[0]*m_tplSize[1];i++){
		for(int j=0;j<m_numTpl+1+m_tplSize[0]*m_tplSize[1]*m_numChannel;j++){
			fprintf(file,"%.4f ",m_temp(i,j));
		}
		fprintf(file,"\n");
	}
	fclose(file);
#endif
	
}

void L1APG::Whitening(MatrixXd & T){

	int row = T.rows();
	int col = T.cols();
	
	if(row<2)
		abortError(__LINE__, __FILE__, "The size of the feature cannot be less than 2."); 
	if(col<1)
		abortError(__LINE__, __FILE__, "At least one instance is required.");

	double tp_sum,tp_mean, tp_std;
	for(int j=0;j<col;j++){
		tp_sum=tp_mean=tp_std=0.0;
		for(int i=0;i<row;i++){
			tp_sum+=T(i,j);
		}
		tp_mean=tp_sum/row;

		for(int i=0;i<row;i++){
			tp_std += (T(i,j)-tp_mean)*(T(i,j)-tp_mean);
		}
		tp_std = tp_std/(row-1);
		tp_std = sqrt(tp_std);

		for(int i=0;i<row;i++){
			if (abs(tp_std)>1e-10){
				T(i,j) = (T(i,j)-tp_mean)/tp_std;
			}
			else{
				T(i,j) = 0;
			}
		}
	}
}

void L1APG::Whitening(double * feature, int size){
	
	if(size<2)
		abortError(__LINE__, __FILE__, "The size of the feature cannot be less than 2."); 
	
	double tp_sum,tp_mean, tp_std;
	tp_sum=tp_mean=tp_std=0.0;
	
	for(int i=0;i<size;i++){
		tp_sum+=feature[i];
	}
	tp_mean=tp_sum/size;
	
	for(int i=0;i<size;i++){
		tp_std += (feature[i]-tp_mean)*(feature[i]-tp_mean);
	}
	tp_std = tp_std/(size-1);
	tp_std = sqrt(tp_std);

	for(int i=0;i<size;i++){
		if (abs(tp_std)>1e-10){
			feature[i] = (feature[i]-tp_mean)/tp_std;
		}
		else{
			feature[i] = 0;
		}
	}
}

void L1APG::L2Normalize(MatrixXd & T){

	int row = T.rows();
	int col = T.cols();

	if(col<1)
		abortError(__LINE__, __FILE__, "At least one instance is required.");

	double value;

	for(int j=0;j<col;j++){
		value = 0.0;
		for(int i=0;i<row;i++){
			value += T(i,j)*T(i,j);
		}
		value = sqrt(value);
		if(value==0){
			continue;
		}
		for(int i=0;i<row;i++){
			T(i,j) = T(i,j)/value;
		}
	}

}

void L1APG::L2Normalize(double * feature, int size){
	
	double value;
	value = 0.0;
	for(int i=0;i<size;i++){
		value += feature[i]*feature[i];
	}
	value = sqrt(value);
	if(value==0){
		return;
	}
	for(int i=0;i<size;i++){
		feature[i]=feature[i]/value;
	}
}

void L1APG::DrawSamples(const cv::Mat & frame){
	
	
	/*double ox,oy,owidth,oheight;
	ox = m_initStatus[0];
	oy = m_initStatus[1];
	owidth = m_initStatus[2];
	oheight = m_initStatus[3];*/

	double std;
	std = 1.0;
//	cout<<m_samples.rows()<<endl;
//	cout<<m_samples.cols()<<endl;
	for(int i=0;i<m_numSample;i++){
		double scale;
		double r1,r2,r3;
		r1 = m_rng.gaussian(std);
		r2 = m_rng.gaussian(std);
		r3 = m_rng.gaussian(std);
		m_samples(i,0) = m_samples(i,0)+r1*m_affStd[0];
		m_samples(i,1) = m_samples(i,1)+r2*m_affStd[1];
		scale = 1+r3*m_affStd[2];
		if(scale<=0){
			scale = 1;
		}
		m_samples(i,2) = m_samples(i,2)*scale;
		m_samples(i,3) = m_samples(i,3)*scale;
	//	cout<<m_samples(i,0)<<" "<<m_samples(i,1)<<" "<<m_samples(i,2)<<" "<<m_samples(i,3)<<endl;
	}

#ifdef IS_DEBUG
	FILE * file = fopen("samples.txt","w+");

	for(int i=0;i<m_numSample;i++){
		fprintf(file,"%.4f %.4f %.4f %.4f\n",m_samples(i,0),m_samples(i,1),m_samples(i,2),m_samples(i,3));
	}
	fclose(file);
#endif

	ImageRep imgRep(frame,m_isColor);
	if(!m_isColor){
		//MatrixXd T(m_tplSize[1]*m_tplSize[2],10);

#ifdef IS_DEBUG
		Mat img_org = imgRep.GetImage(0);
		file = fopen("img.txt","w+");
		for(int i=0;i<img_org.rows;i++){
			for(int j=0;j<img_org.cols;j++){
				fprintf(file,"%d ",(int)img_org.at<uchar>(i,j));
			}
			fprintf(file,"\n");
		}
		fclose(file);
#endif
		for(int i=0;i<m_numSample;i++){
			cv::Rect roi(m_samples(i,0), m_samples(i,1), m_samples(i,2), m_samples(i,3));
			CheckRoi(roi);
			Mat img_roi = imgRep.GetImage(0)(roi);
			Mat im(m_tplSize[0], m_tplSize[1], CV_8UC1);
			cv::resize(img_roi, im, im.size());
			//double * feature = new double[im.rows*im.cols];
			int inx = 0;
			for(int j=0;j<im.rows;j++){
				for(int k=0;k<im.cols;k++){
					//feature[inx++] = (int)im.at<uchar>(j,k);
					m_sampleFeature(inx++,i) = (int)im.at<uchar>(j,k);
				}
			}
			//Whitening(feature,im.rows*im.cols);
			//L2Normalize(feature,im.rows*im.cols);
			//for(int j=0;j<im.rows*im.cols;j++){
			//	m_sampleFeature(j,i) = feature[j];
			//}
			//delete[] feature;
		}
		/*FILE * file = fopen("samples.txt","w+");
		for(int i=0;i<m_tplSize[0]*m_tplSize[1];i++){
			for(int j=0;j<m_numSample;j++){
				fprintf(file,"%.4f ",m_sampleFeature(i,j));
			}
			fprintf(file,"\n");
		}
		fclose(file);*/
	}
	else{
		int ginx=0;
		for(int nc=0;nc<m_numChannel;nc++){
			for(int i=0;i<m_numSample;i++){
				cv::Rect roi(m_samples(i,0), m_samples(i,1), m_samples(i,2), m_samples(i,3));
				CheckRoi(roi);
				Mat img_roi = imgRep.GetOppoImage(nc)(roi);
				Mat im(m_tplSize[0], m_tplSize[1], CV_32FC1);
				cv::resize(img_roi, im, im.size());
				//double * feature = new double[im.rows*im.cols];
				int inx = 0;
				for(int j=0;j<im.rows;j++){
					for(int k=0;k<im.cols;k++){
						//feature[inx++] = im.at<float>(j,k);
						m_sampleFeature(inx+ginx,i) = im.at<float>(j,k);
						++inx;
					}
				}
				//Whitening(feature,im.rows*im.cols);
				//L2Normalize(feature,im.rows*im.cols);
				//for(int j=0;j<im.rows*im.cols;j++){
				//	m_sampleFeature(j+ginx,i) = feature[j];
				//}
				//delete[] feature;
			}
			ginx+=m_tplSize[0]*m_tplSize[1];
		}
	}
	Whitening(m_sampleFeature);
	L2Normalize(m_sampleFeature);
}

MatrixXd L1APG::LSS(){

#ifdef IS_DEBUG
	FILE * file = fopen("sampleFeature.txt","w+");
	for(int i=0;i<m_tplSize[0]*m_tplSize[1];i++){
		for(int j=0;j<m_numSample;j++){
			fprintf(file,"%.4f ",m_sampleFeature(i,j));
		}
		fprintf(file,"\n");
	}
	fclose(file);


	file = fopen("templates.txt","w+");
	for(int i=0;i<m_tplSize[0]*m_tplSize[1];i++){
		for(int j=0;j<m_numTpl+1;j++){
			fprintf(file,"%.4f ",m_temp2(i,j));
		}
		fprintf(file,"\n");
	}
	fclose(file);
#endif
	assert(m_temp2.cols()==m_numTpl+1);
	Mat templates(m_temp2.rows(),m_temp2.cols(),CV_64FC1);
	Mat samples(m_sampleFeature.rows(),m_sampleFeature.cols(),CV_64FC1);
	Mat rt(m_numTpl+1,m_numSample,CV_64FC1);

	for(int i=0;i<m_temp2.rows();i++){
		for(int j=0;j<m_temp2.cols();j++){
			templates.at<double>(i,j) = m_temp2(i,j);
		}
	}
	for(int i=0;i<m_sampleFeature.rows();i++){
		for(int j=0;j<m_sampleFeature.cols();j++){
			samples.at<double>(i,j) = m_sampleFeature(i,j);
		}
	}

	solve(templates, samples, rt, DECOMP_SVD);

	MatrixXd result(m_numTpl+1,m_numSample);

	for(int i=0;i<m_numSample;i++){
		for(int j=0;j<m_numTpl+1;j++){
			result(j,i)=rt.at<double>(j,i);
		}
	}
	
	
#ifdef IS_DEBUG
	file = fopen("result.txt","w+");
	for(int i=0;i<m_numTpl+1;i++){
		for(int j=0;j<m_numSample;j++){
			fprintf(file,"%.4f ", result(i,j));
		}
		fprintf(file,"\n");
	}
	fclose(file);
#endif
	/*file = fopen("result_2.txt","w+");
	for(int i=0;i<m_numTpl+1;i++){
		for(int j=0;j<m_numSample;j++){
			fprintf(file,"%.4f ", rt.at<double>(i,j));
		}
		fprintf(file,"\n");
	}
	fclose(file);*/

	return result;
}

//MatrixXd L1APG::LSS(){
//
//	FILE * file = fopen("samples.txt","w+");
//	for(int i=0;i<m_tplSize[0]*m_tplSize[1];i++){
//		for(int j=0;j<m_numSample;j++){
//			fprintf(file,"%.4f ",m_sampleFeature(i,j));
//		}
//		fprintf(file,"\n");
//	}
//	fclose(file);
//
//	file = fopen("templates.txt","w+");
//	for(int i=0;i<m_tplSize[0]*m_tplSize[1];i++){
//		for(int j=0;j<m_numTpl+1;j++){
//			fprintf(file,"%.4f ",m_temp2(i,j));
//		}
//		fprintf(file,"\n");
//	}
//	fclose(file);
//	
//	MatrixXd result(m_numTpl+1,m_numSample);
//	
//	int dim;
//	dim = m_tplSize[0]*m_tplSize[1]*m_numChannel;
//	for(int i=0;i<m_numSample;i++){
//		VectorXd b(dim);
//		for(int j=0;j<dim;j++){
//			b(j)=m_sampleFeature(j,i);
//		//	cout<<b(j)<<endl;
//		}
//		
//		VectorXd rt(m_numTpl+1);
//		rt = m_temp2.topLeftCorner(15,11).jacobiSvd(ComputeThinU | ComputeThinV).solve(b.topLeftCorner(15,1));
//		cout<<rt;
//		if(rt.size()!=m_numTpl+1){
//			abortError( __LINE__, __FILE__,"The size of the result need to be consistent with the input."); 
//		}
//		for(int j=0;j<m_numTpl+1;j++){
//			result(j,i)=rt(j);
//			cout<<result(j,i)<<endl;
//		}
//	}
//
//	file = fopen("result.txt","w+");
//	for(int i=0;i<m_numTpl+1;i++){
//		for(int j=0;j<m_numSample;j++){
//			fprintf(file,"%.4f ",result(i,j));
//		}
//		fprintf(file,"\n");
//	}
//	fclose(file);
//
//	return result;
//}

VectorXd L1APG::L2Error(){
	
	VectorXd err(m_numSample);
	MatrixXd coeffMatrix = LSS();
	
	MatrixXd recons(m_temp2.rows(),coeffMatrix.cols());
	MatrixXd diff(m_sampleFeature.rows(),m_sampleFeature.cols());
	recons = m_temp2*coeffMatrix;
	diff = m_sampleFeature-recons;

	int dim;
	dim = m_tplSize[0]*m_tplSize[1]*m_numChannel;

	for(int i=0;i<m_numSample;i++){
		double value=0.0;
		for(int j=0;j<dim;j++){
			value+=diff(j,i)*diff(j,i);
		}
		err(i)=sqrt(value);
	}
	return err;
}

double L1APG::L2Norm(VectorXd feat){
	
	double value=0;
	int size = feat.size();
	for(int i=0;i<size;i++){
		value+=feat(i)*feat(i);
	}
	value=sqrt(value);
	return value;
}

double L1APG::L1Norm(VectorXd feat){
	
	double value=0;
	int size = feat.size();
	for(int i=0;i<size;i++){
		value+=fabs(feat(i));
	}
	return value;
}

double L1APG::LSum(VectorXd feat){
	
	double value=0;
	int size = feat.size();
	for(int i=0;i<size;i++){
		value+=feat(i);
	}
	return value;
}

double L1APG::InnerProd(VectorXd feat_1,VectorXd feat_2){
	
	int size_1,size_2;
	size_1=feat_1.size();
	size_2=feat_2.size();
	assert(size_1==size_2);
	
	double value=0.0;
	for(int i=0;i<size_1;i++){
		value+=feat_1(i)*feat_2(i);
	}
	return value;
}

VectorXd L1APG::APG_Solver(const MatrixXd &b,const MatrixXd & A,const MatrixXd & y,const MatrixXd & D,vector<double> & funVal,vector<double> & funVal2){

	/*
	%% Object: c = argmin (1/2)\|y-Dx\|_2^2+lambda\|x\|_1+\mu\|x_I\|_2^2 
	%%                s.t. x_T >0 
	%% input arguments:
	%%         b -------  D'*y transformed object vector
	%%         A -------  D'*D transformed dictionary
	%%         para ------ Lambda: Sparsity Level
	%%                     (lambda1: template set; lambda2:trivial template; lambda3:\mu)
	%%                     Lip: Lipschitz Constant for F(x)
	%%                     Maxit: Maximal Iteration number
	%%                     nT: number of templates
	%% output arguments:
	%%         c ------  output Coefficient vetor*/

	int colDim = A.rows();
	VectorXd xPrev(colDim,1);
	VectorXd x(colDim,1);
	for(int i=0;i<colDim;i++){
		xPrev(i,0)=0;
		x(i,0)=0;
	}
	double tPrev=1.0;
	double t=1.0;
	VectorXd tLambda(colDim,1);
	for(int i=0;i<m_numTpl;i++){
		tLambda(i,0)=m_lambda[0];
	}
	tLambda(colDim-1,0)=m_lambda[0];

	double eta=2;
	double Lk=1;
	double Lkp=1;

	for(int i=0;i<m_maxIt;i++){
		double tem_t = (tPrev-1)/t;
		VectorXd tem_y(colDim,1);
		for(int j=0;j<colDim;j++){
			tem_y(j,0)=(1+tem_t)*x(j,0)-tem_t*xPrev(j,0);
		}
		for(int j=m_numTpl;j<colDim-1;j++){
			tLambda(j,0)=m_lambda[2]*tem_y(j,0);
		}
		VectorXd tem_gradient;
		tem_gradient = A*tem_y;
		tem_gradient = tem_gradient - b;
		tem_gradient = tem_gradient + tLambda;

		
		Lk = m_lip;
		VectorXd tem_dif;
		tem_dif = tem_y-tem_gradient/Lk;
		for(int j=0;j<m_numTpl;j++){
			if(tem_dif(j,0)>0){
				x(j,0)=tem_dif(j,0);
			}
			else{
				x(j,0)=0;
			}
		}
		if(tem_dif(colDim-1,0)>0){
			x(colDim-1,0)=tem_dif(colDim-1,0);
		}
		else{
			x(colDim-1,0)=0;
		}
		for(int j=m_numTpl;j<colDim-1;j++){
			double tp_val = m_lambda[1]/Lk;
			x(j,0)=MAX(tem_dif(j,0)-tp_val,0)-MAX(-tem_dif(j,0)-tp_val,0);
		}
		
		double Fk=0.0;

		double val_1,val_2,val_3,val_4,val_5;

		//			cout<<y<<endl;
		//			cout<<"----------------------------------------"<<endl;
		//			cout<<D*x;
		/*val_1=val_2=val_3=val_4=val_5=0.0;
		val_1 = L2Norm(y-D*x);
		val_2 = LSum(x.segment(0,m_numTpl));
		val_3 = LSum(x.segment(x.size()-1,1));
		val_4 = L2Norm(x.segment(m_numTpl,x.size()-m_numTpl-1));
		val_5 = L1Norm(x.segment(m_numTpl,x.size()-m_numTpl-1));
		Fk = 0.5*val_1*val_1+m_lambda[0]*(val_2+val_3)+0.5*m_lambda[2]*val_4*val_4+m_lambda[1]*val_5;


		funVal.push_back(Fk);*/
		Lkp = Lk;
		xPrev = x;
		tPrev = t;
		t=(1+sqrt(1+4*t*t))/2;
	}
	return x;

}

VectorXd L1APG::APG_BT(const MatrixXd &b,const MatrixXd & A,const MatrixXd & y,const MatrixXd & D,vector<double> & funVal,vector<double> & funVal2){
	/*
	%% Object: c = argmin (1/2)\|y-Dx\|_2^2+lambda\|x\|_1+\mu\|x_I\|_2^2 
%%                s.t. x_T >0 
%% input arguments:
%%         b -------  D'*y transformed object vector
%%         A -------  D'*D transformed dictionary
%%         para ------ Lambda: Sparsity Level
%%                     (lambda1: template set; lambda2:trivial template; lambda3:\mu)
%%                     Lip: Lipschitz Constant for F(x)
%%                     Maxit: Maximal Iteration number
%%                     nT: number of templates
%% output arguments:
%%         c ------  output Coefficient vetor*/

	int colDim = A.rows();
	VectorXd xPrev(colDim,1);
	VectorXd x(colDim,1);
	for(int i=0;i<colDim;i++){
		xPrev(i,0)=0;
		x(i,0)=0;
	}
	double tPrev=1.0;
	double t=1.0;
	VectorXd tLambda(colDim,1);
	for(int i=0;i<m_numTpl;i++){
		tLambda(i,0)=m_lambda[0];
	}
	tLambda(colDim-1,0)=m_lambda[0];
	
	double eta=2;
	double Lk=1;
	double Lkp=1;

	for(int i=0;i<m_maxIt;i++){
		double tem_t = (tPrev-1)/t;
		VectorXd tem_y(colDim,1);
		for(int j=0;j<colDim;j++){
			tem_y(j,0)=(1+tem_t)*x(j,0)-tem_t*xPrev(j,0);
		}
		for(int j=m_numTpl;j<colDim-1;j++){
			tLambda(j,0)=m_lambda[2]*tem_y(j,0);
		}
		VectorXd tem_gradient;
		tem_gradient = A*tem_y;
		tem_gradient = tem_gradient - b;
		tem_gradient = tem_gradient + tLambda;
		double ik=0.0;
		double Fk=0.0;
		double fkk=0.0;
		double val_1,val_2,val_3,val_4,val_5;
		double gk=0.0;
		double Qk = 0.0;
		while(true){
			Lk = pow(eta,ik)*Lkp;
			VectorXd tem_dif;
			tem_dif = tem_y-tem_gradient/Lk;
			for(int j=0;j<m_numTpl;j++){
				if(tem_dif(j,0)>0){
					x(j,0)=tem_dif(j,0);
				}
				else{
					x(j,0)=0;
				}
			}
			if(tem_dif(colDim-1,0)>0){
				x(colDim-1,0)=tem_dif(colDim-1,0);
			}
			else{
				x(colDim-1,0)=0;
			}
			for(int j=m_numTpl;j<colDim-1;j++){
				double tp_val = m_lambda[1]/Lk;
				x(j,0)=MAX(tem_dif(j,0)-tp_val,0)-MAX(-tem_dif(j,0)-tp_val,0);
			}
			Fk=0.0;
			fkk=0.0;
			gk=0.0;
			Qk = 0.0;
			
//			cout<<y<<endl;
//			cout<<"----------------------------------------"<<endl;
//			cout<<D*x;
			val_1=val_2=val_3=val_4=val_5=0.0;
			val_1 = L2Norm(y-D*x);
			val_2 = LSum(x.segment(0,m_numTpl));
			val_3 = LSum(x.segment(x.size()-1,1));
			val_4 = L2Norm(x.segment(m_numTpl,x.size()-m_numTpl-1));
			val_5 = L1Norm(x.segment(m_numTpl,x.size()-m_numTpl-1));
			Fk = 0.5*val_1*val_1+m_lambda[0]*(val_2+val_3)+0.5*m_lambda[2]*val_4*val_4+m_lambda[1]*val_5;
			
			gk=m_lambda[1]*val_5;

			val_1 = L2Norm(y-D*tem_y);
			val_2 = LSum(tem_y.segment(0,m_numTpl));
			val_3 = LSum(tem_y.segment(tem_y.size()-1,1));
			val_4 = L2Norm(tem_y.segment(m_numTpl,tem_y.size()-m_numTpl-1));
			fkk = 0.5*val_1*val_1+m_lambda[0]*(val_2+val_3)+m_lambda[2]*0.5*val_4*val_4;

			val_1 = L2Norm(x-tem_y);
			Qk = fkk+InnerProd(x-tem_y,tem_gradient)+0.5*Lk*val_1*val_1+gk;

			if(Fk<=Qk)
				break;
			else
				++ik;
		}
		funVal.push_back(Fk);
		Lkp = Lk;
		xPrev = x;
		tPrev = t;
		t=(1+sqrt(1+4*t*t))/2;
		if(i>0){
			if(fabs(funVal[i]-funVal[i-1])<=m_tol)
				break;
		}
	}
	return x;
}

void L1APG::Resample(vector<double> plh, const double * curStatus){
	
	//MatrixXd samples = m_samples;
	MatrixXd samples(m_samples.rows(),m_samples.cols());
	int sz1,sz2;
	sz1 = m_samples.rows();
	sz2 = m_samples.cols();
	for(int i=0;i<sz1;i++){
		for(int j=0;j<sz2;j++){
			samples(i,j) = m_samples(i,j);
		}
	}

	assert(samples.rows()==m_numSample);
	assert(plh.size()==m_numSample);
	double sum=0.0;
	for(int i=0;i<plh.size();i++){
		sum+=plh[i];
	}
	if(sum==0){
		for(int i=0;i<m_numSample;i++){
			m_samples(i,0)=curStatus[0];
			m_samples(i,1)=curStatus[1];
			m_samples(i,2)=curStatus[2];
			m_samples(i,3)=curStatus[3];
		}
		return;
	}
	double tp_sum=0.0;
	for(int i=0;i<plh.size();i++){
		plh[i] = plh[i]/sum;
		tp_sum+=plh[i];
	}
//	cout<<tp_sum<<endl;
	int total=0;
	for(int i=0;i<m_numSample;i++){
		int count = plh[i]*m_numSample;
		for(int j=0;j<count;j++){
			m_samples(total,0)=samples(i,0);
			m_samples(total,1)=samples(i,1);
			m_samples(total,2)=samples(i,2);
			m_samples(total,3)=samples(i,3);
			++total;
		}
		if(total>=m_numSample)
			break;
	}
	if(total<=m_numSample-1){
		for(int i=total;i<m_numSample;i++){
			m_samples(i,0)=curStatus[0];
			m_samples(i,1)=curStatus[1];
			m_samples(i,1)=curStatus[2];
			m_samples(i,1)=curStatus[3];
		}
	}
}

double L1APG::VectorAngle(VectorXd v1,VectorXd v2) {
	
	assert(v1.size()==v2.size());
	double d1,d2,prod;
	d1=d2=prod=0.0;
	int size=v1.size();
	for(int i=0;i<size;i++){
		d1+=v1(i)*v1(i);
		d2+=v2(i)*v2(i);
		prod+=v1(i)*v2(i);
	}
	d1=sqrt(d1);
	d2=sqrt(d2);
	double cosValue = prod/(d1*d2);
	double angle;
	//assert(cosValue>=-1);
	//assert(cosValue<=1);
	angle = acos(cosValue);
	angle = angle*180/PI;

	return angle;
}

void L1APG::Track(const cv::Mat & frame){
	
	if(!m_initialized){
		cout<<"The tracker need to be initialized first!"<<endl;
		return; 
	}

	//ImageRep imgRep(frame,m_isColor);
	
	DrawSamples(frame);
//	long int startT=clock();
	VectorXd l2err = L2Error();
//	long int endT=clock();
//	m_time+=endT-startT;
	vector<Element> elements;
	for(int i=0;i<l2err.size();i++){
		double value=l2err(i)*l2err(i);
		value = exp(-m_alpha*value);
		Element elem(value,i);
		elements.push_back(elem);
	}
	sort(elements.begin(),elements.end());
	vector<double> plh(elements.size(),0);

	double tau=0;
	int total=1;
	int inx=elements.size()-1;
	double eta_max = -INF;
	int id_max = -1;
	VectorXd c_max;
	int cnt=0;
	while(total<=m_numSample&&elements[inx].value>=tau){
		++cnt;
		MatrixXd tp_y(m_sampleFeature.rows(),1);
		int tp_inx = elements[inx].index;
		double tp_sum=0.0;
		for(int i=0;i<m_sampleFeature.rows();i++){
			tp_y(i,0)=m_sampleFeature(i,tp_inx);
			tp_sum+=tp_y(i,0);
		}
		if(tp_sum==0.0){
			++total;
			--inx;
			continue;
		}
		vector<double> funVal;
		vector<double> funVal2;
//		cout<<tp_y<<endl;
//		cout<<"----------------------"<<endl;
		
//		long int startT=clock();
//		MatrixXd tp_matrix = m_tempTran*tp_y;
//		long int endT = clock();
		VectorXd coeff;
		if(!m_isColor){
			coeff = APG_Solver(m_temp.transpose()*tp_y,m_dict,tp_y,m_temp,funVal,funVal2);
		}
		else{
			coeff = APG_BT(m_temp.transpose()*tp_y,m_dict,tp_y,m_temp,funVal,funVal2);
		}
		
//		VectorXd coeff = APG_Solver(tp_matrix,m_dict,tp_y,m_temp,funVal,funVal2);
		
//		m_time+=endT-startT;
		VectorXd coeff_T(m_numTpl+1);
		for(int i=0;i<m_numTpl;i++){
			coeff_T(i)=coeff(i);
		}
		coeff_T(m_numTpl)=coeff(coeff.size()-1);
		double tp_err;
		tp_err = L2Norm(tp_y-m_temp2*coeff_T);
		tp_err = tp_err*tp_err;
		plh[tp_inx]=exp(-m_alpha*tp_err);
		tau+=plh[tp_inx]/(2*m_numSample-1);
		if(LSum(coeff_T.segment(0,m_numTpl))<0){
			continue;
		}
		else if(plh[tp_inx]>eta_max){
			eta_max = plh[tp_inx];
			id_max = tp_inx;
			c_max = coeff;
		}
#ifdef IS_DEBUG
		cout<<total<<" "<<tp_inx<<" "<<tp_err<<" "<<plh[tp_inx]<<" "<<elements[inx].value<<" "<<tau<<" "<<funVal.size()<<endl;
#endif
		--inx;
		++total;
	}
//	cout<<cnt<<endl;
	if(id_max>=0){
		m_curStatus[0] = m_samples(id_max,0);
		m_curStatus[1] = m_samples(id_max,1);
		m_curStatus[2] = m_samples(id_max,2);
		m_curStatus[3] = m_samples(id_max,3);
	}
	Resample(plh,m_curStatus);
#ifdef IS_DEBUG
	FILE * file = fopen("re_sample.txt","w+");
	for(int i=0;i<m_numSample;i++){
		fprintf(file,"%.4f %.4f %.4f %.4f\n",m_samples(i,0),m_samples(i,1),m_samples(i,2),m_samples(i,3));
	}
	fclose(file);
#endif
	int index_max=-1;
	double tp_max=-INF;
	for(int i=0;i<m_numTpl;i++){
		if(c_max(i)>tp_max){
			tp_max = c_max(i);
			index_max = i;
		}
	}
//	cout<<c_max<<endl;
	double minAngle;
	minAngle = VectorAngle(m_sampleFeature.col(id_max),m_temp.col(index_max));
	
	m_occNf-=1;
	//double level = 0.03;
	if(minAngle >= m_angleThreshold&&m_occNf<0){
		//cout<<"Update!"<<endl;
		int dim = m_tplSize[0]*m_tplSize[1];
		vector<VectorXd> cset;
		int tp_index = m_numTpl;
		for(int i=0;i<m_numChannel;i++){
			VectorXd cTr = c_max.segment(tp_index,dim);
			cset.push_back(cTr);
			tp_index+=dim;
		}
		vector<Mat> mapSet;
		for(int i=0;i<m_numChannel;i++){
			Mat tp_mat = Mat::zeros(m_tplSize[0],m_tplSize[1],CV_8UC1);
			VectorXd cTr = cset[i];
			/*cout<<"-----------"<<endl;
			cout<<cTr<<endl;
			cout<<"-----------"<<endl;*/
			assert(cTr.size()==dim);
			tp_index = 0;
			for(int j=0;j<m_tplSize[0];j++){
				for(int k=0;k<m_tplSize[1];k++){
					double tp_val = cTr(tp_index);
					if(fabs(tp_val)>m_level){
						tp_mat.at<uchar>(j,k)=1;
					}
					else{
						tp_mat.at<uchar>(j,k)=0;
					}
					++tp_index;
				//	printf("%.4f ",tp_val);
				}
				//printf("\n");
			}
			mapSet.push_back(tp_mat);
		}
		double maxArea = 0;
		for(int i=0;i<m_numChannel;i++){
			Mat tp_map = mapSet[i];
			Mat mapDilate;
			dilate(tp_map,mapDilate,Mat());
			vector<vector<Point> > contours;
			findContours(mapDilate,contours,CV_RETR_CCOMP,CV_CHAIN_APPROX_NONE);
			for(size_t j=0;j<contours.size();j++){
				double area = contourArea(contours[j]);
				if(area>maxArea)
					maxArea = area;
			}
		}
		if(maxArea<0.25*dim){
			//cout<<"Replace template"<<endl;
			//cout<<"Angle:"<<minAngle<<" Max Area:"<<maxArea<<endl;
			int index_min=-1;
			double tp_min = INF;
			for(int i=0;i<m_numTpl;i++){
				if(c_max(i)<tp_min){
					tp_min = c_max(i);
					index_min = i;
				}
			}
			assert(m_sampleFeature.rows()==m_temp.rows());
			int tp_size = m_sampleFeature.rows();
			for(int i=0;i<tp_size;i++){
				m_temp(i,index_min) = m_sampleFeature(i,id_max);
				m_temp2(i,index_min) = m_sampleFeature(i,id_max);
			}
			m_tempTran = m_temp.transpose();
			m_dict = m_temp.transpose()*m_temp;
		}
		else{
			m_occNf = 5;
			m_lambda[2] = 0;
		}
	}
	else if(m_occNf<0){
		m_lambda[2] =  m_mu;
	}
}

FloatRect L1APG::GetBB(){
	
	return FloatRect(m_curStatus[0],m_curStatus[1],m_curStatus[2],m_curStatus[3]);
}
