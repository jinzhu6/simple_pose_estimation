#include "simple_pose_estimator.h"

// https://hub.packtpub.com/reconstructing-3d-scenes/
// 

using namespace cv;
using namespace std;

// Checks if a matrix is a valid rotation matrix. To be a rotation
// matrix, a matrix should meet the following two criteria: 1) R^{T}R
// = I and 2) det(R)=1
bool isRotationMatrix(Mat &R)
{
  Mat Rt;
  transpose(R, Rt);
  Mat shouldBeIdentity = Rt * R;
  std::cout <<"The following should be an identity matrix, R^{T}R=" << std::endl
	    << shouldBeIdentity << std::endl;
  Mat I = Mat::eye(3,3, shouldBeIdentity.type());
  
  return  norm(I, shouldBeIdentity) < 1e-6;
}

// Calculates rotation matrix to euler angles
// The result is the same as MATLAB except the order
// of the euler angles ( x and z are swapped ).
//
// [Determine yaw, pitch and roll directly from rotation
// matrix](http://planning.cs.uiuc.edu/node103.html)
Vec3f rotationMatrixToEulerAngles(Mat &R)
{
  assert(isRotationMatrix(R));
    
  float sy = sqrt(R.at<double>(0,0) * R.at<double>(0,0) +  R.at<double>(1,0) * R.at<double>(1,0) );

  bool singular = sy < 1e-6; // If

  float x, y, z;
  if (!singular)
  {
    x = atan2(R.at<double>(2,1) , R.at<double>(2,2));
    y = atan2(-R.at<double>(2,0), sy);
    z = atan2(R.at<double>(1,0), R.at<double>(0,0));
  }else
  {
    x = atan2(-R.at<double>(1,2), R.at<double>(1,1));
    y = atan2(-R.at<double>(2,0), sy);
    z = 0;
  }

  return Vec3f(x, y, z);
}

// Calculates rotation matrix given euler angles.
Mat eulerAnglesToRotationMatrix(Vec3f &theta)
{
  // Calculate rotation about x axis
  Mat R_x = (Mat_<double>(3,3) <<
	     1,       0,              0,
	     0,       cos(theta[0]),   -sin(theta[0]),
	     0,       sin(theta[0]),   cos(theta[0])
	     );
  
  // Calculate rotation about y axis
  Mat R_y = (Mat_<double>(3,3) <<
	     cos(theta[1]),    0,      sin(theta[1]),
	     0,               1,      0,
	     -sin(theta[1]),   0,      cos(theta[1])
	     );
  
  // Calculate rotation about z axis
  Mat R_z = (Mat_<double>(3,3) <<
	     cos(theta[2]),    -sin(theta[2]),      0,
	     sin(theta[2]),    cos(theta[2]),       0,
	     0,               0,                  1);
  
  
  // Combined rotation matrix
  Mat R = R_z * R_y * R_x;
  
  return R;
}

int main(int argc, char** argv)
{
  Mat input_img;

  input_img = imread("../bench_img.jpg", IMREAD_COLOR);
  //std::cout << "Type of input image=" << input_img.type() << std::endl;
  //std::cout << "Size of input image=" << input_img.size() << std::endl;
  
  if (!input_img.data)
  {
    std::cout << "Error loading input image" << std::endl; return -1;
  }

  //Input object points
  std::vector<cv::Point3f> objectPoints;
  objectPoints.push_back(cv::Point3f(0, 45, 0));
  objectPoints.push_back(cv::Point3f(242.5, 45, 0));
  objectPoints.push_back(cv::Point3f(242.5, 21, 0));
  objectPoints.push_back(cv::Point3f(0, 21, 0));
  objectPoints.push_back(cv::Point3f(0, 9, -9));
  objectPoints.push_back(cv::Point3f(242.5, 9, -9));
  objectPoints.push_back(cv::Point3f(242.5, 9, 44.5));
  objectPoints.push_back(cv::Point3f(0, 9, 44.5));

  //Input image points
  std::vector<cv::Point2f> imagePoints;
  imagePoints.push_back(cv::Point2f(203, 165));
  imagePoints.push_back(cv::Point2f(572, 170));
  imagePoints.push_back(cv::Point2f(570, 227));
  imagePoints.push_back(cv::Point2f(575, 246));
  imagePoints.push_back(cv::Point2f(519, 292));
  imagePoints.push_back(cv::Point2f(157, 240));
  imagePoints.push_back(cv::Point2f(218, 215));
  imagePoints.push_back(cv::Point2f(205, 201));

  for(int i=0; i < imagePoints.size(); i++)
  {
    circle(input_img, imagePoints[i], 3, Scalar(0, 0, 255), -1);
  }
  
  //fx=409 pixels; fy=408 pixels; u0=237; and v0=171. 
  double fx = 409, fy = 408, u0=237, v0=171;
  Mat camera_matrix = (Mat_ <double>(3,3) << fx, 0, u0, 0, fy, v0, 0, 0, 1);
  cv::Matx33d cam_matrix(camera_matrix);
  
  // Assuming no distortion
  Mat dist_coeffs = Mat::zeros(4,1,cv::DataType<double>::type);
  std::cout << "Camera_matrix " << endl << camera_matrix << endl;
					
  // Get the camera pose from 3D/2D points
  cv::Mat rvec, tvec;
  cv::solvePnP(objectPoints, imagePoints,  // corresponding 3D/2D pts 
	       camera_matrix, dist_coeffs, // calibration 
	       rvec, tvec);    // output pose

  // Convert rotation vector, rvec (1x3), to 3D rotation matrix (3x3)
  cv::Mat rotation;
  cv::Rodrigues(rvec, rotation);

  // Calculate Euler angles (1x3) from rotation matrix (3x3)
  Vec3f e = rotationMatrixToEulerAngles(rotation);
  
  // Calculate rotation matrix (3x3) from the calculated Euler angles for
  // verification
  Mat R = eulerAnglesToRotationMatrix(e);
    
  cout << endl << "Euler Angles from R" << endl << e << endl;
  cout << endl << "R from the Euler Angles: " << endl << R << endl << endl;

  std::cout << "Rotation vector " << std::endl << rvec << std::endl;
  std::cout << endl << "R, " << std::endl << rotation << std::endl;
  std::cout << "T, " << std::endl << tvec << std::endl;

  namedWindow("Input Image");  
  cv::imshow("Input Image", input_img);
  waitKey();

  // Visualization of point clouds  
  cv::viz::Viz3d visualizer("Viz Window");
  visualizer.setBackgroundColor(cv::viz::Color::white());

  // Create a virtual camera
  cv::viz::WCameraPosition cam(cam_matrix,
			       input_img,
			       30.0,
			       cv::viz::Color::black());

  // Add the virtual camera to the environment
  visualizer.showWidget("Camera", cam);

  // Create a virtual bench from cuboids
  cv::viz::WCube plane1(Point3f(0.0, 45.0, 0.0), 
			Point3f(242.5, 21.0, -9.0), 
			true,  // show wire frame 
			cv::viz::Color::blue());
  
  plane1.setRenderingProperty(cv::viz::LINE_WIDTH, 4.0);

  cv::viz::WCube plane2(Point3f(0.0, 9.0, -9.0), 
			Point3f(242.5, 0.0, 44.5), 
			true,  // show wire frame 
			cv::viz::Color::blue());
  
  plane2.setRenderingProperty(cv::viz::LINE_WIDTH, 4.0);
  
  // Add the virtual objects to the environment
  visualizer.showWidget("top", plane1);
  visualizer.showWidget("bottom", plane2);  

  // Move the bench	
  Affine3d pose(rotation, tvec);
  visualizer.setWidgetPose("top", pose);
  visualizer.setWidgetPose("bottom", pose);

  // visualization loop
  while(waitKey(100)==-1 && !visualizer.wasStopped()) {
    visualizer.spinOnce(1,    // pause 1ms 
			true);  // redraw
  }
  
  return 0;
}
