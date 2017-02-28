#include <auv_mission_control/Vision.hpp>

Vision::Vision() {}

Vision::Vision(std::shared_ptr<Camera> cam) : cam_(cam) {}

Vision::~Vision() {}

void Vision::findBuoy(int color) {
  cam_->updateFrames();
  imgOrigFront = cam_->getFront();

  cv::cvtColor(imgOrigFront, imgHlsFront, CV_BGR2HLS);

  if (color == COLOR_RED) {
    ROS_INFO("COLOR IS RED");
    cv::inRange(imgHlsFront, sRedMin, sRedMax, imgThreshFront);
  } else if (color == COLOR_GREEN) {
    cv::inRange(imgHlsFront, sGreenMin, sGreenMax, imgThreshFront);
  } else {
    ROS_ERROR("INVALID COLOR");
  }

  cv::dilate(imgThreshFront, imgThreshFront,
             cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
  cv::erode(imgThreshFront, imgThreshFront,
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

  cv::erode(imgThreshFront, imgThreshFront,
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
  cv::dilate(imgThreshFront, imgThreshFront,
             cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

  cv::dilate(imgThreshFront, imgThreshFront,
             cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7)));

  momentsBuoy = cv::moments(imgThreshFront);

  buoyArea = momentsBuoy.m00;
  buoyCoordX = momentsBuoy.m10 / buoyArea;
  buoyCoordY = momentsBuoy.m01 / buoyArea;

  // here for if we need it
  buoyCoordCorrectedX = buoyCoordX;
  buoyCoordCorrectedY = buoyCoordY;
}

double Vision::getBuoyArea() { return buoyArea; }

double Vision::getBuoyCoordX() { return buoyCoordCorrectedX; }

double Vision::getBuoyCoordY() { return buoyCoordCorrectedY; }

void Vision::findMarker() {
  ROS_INFO("FIND MARKER CALLED");
  cam_->updateFrames();
  imgOrigBottom = cam_->getBottom();
  ROS_INFO("GOT IMAGES");
  cv::cvtColor(imgOrigBottom, imgHlsBottom, CV_BGR2HLS);

  cv::inRange(imgHlsBottom, sRedMin, sRedMax, imgThreshBottom);

  cv::dilate(imgThreshBottom, imgThreshBottom,
             cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
  cv::erode(imgThreshBottom, imgThreshBottom,
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

  cv::erode(imgThreshBottom, imgThreshBottom,
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
  cv::dilate(imgThreshBottom, imgThreshBottom,
             cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

  cv::dilate(imgThreshBottom, imgContoursBottom,
             cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7)));
  cv::dilate(imgThreshBottom, imgThreshBottom,
             cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7)));
  ROS_INFO("THRESHOLDING DONE");

  momentsMarker = cv::moments(imgThreshBottom);

  markerArea = momentsMarker.m00;

  markerCoordX = momentsMarker.m10 / markerArea;
  markerCoordY = momentsMarker.m01 / markerArea;

  // here for if we need it
  markerCoordCorrectedX = markerCoordX;
  markerCoordCorrectedY = markerCoordY;
  ROS_INFO("MOMENTSx");

  cv::findContours(imgContoursBottom, contours, hierarchy, CV_RETR_TREE,
                   CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
  ROS_INFO("found contours");
  numLargeContours = 0;
  largestArea = 0;
  largestContourIndex = 0;
  for (int i = 0; i < contours.size(); i++) {
    double count = contours[i].size();

    if (count < 6)
      continue;

    numLargeContours++;

    double a = contourArea(contours[i], false);
    if (a > largestArea) {
      largestArea = a;
      largestContourIndex = i;
    }
  }
  ROS_INFO("went through contours");

  if (numLargeContours >= 1) {
    ROS_INFO("here");
    double count = contours[largestContourIndex].size();
    if (count > 5)
      markerRect = cv::fitEllipse(contours[largestContourIndex]);

    ROS_INFO("but not here");

    // cv::Point2f rect_points[4];

    // markerRect.points( rect_points );

    double currentAngle = markerRect.angle - 90;
    if (markerRect.size.width < markerRect.size.height)
      currentAngle = 90 + currentAngle;

    averagedAngle = (currentAngle * .1) + (averagedAngle * .9);

  } else {
    ROS_INFO("No Contours greater than 6");
  }
  ROS_INFO("calculated angle");
}

double Vision::getMarkerArea() { return markerArea; }

double Vision::getMarkerCoordX() { return markerCoordCorrectedX; }

double Vision::getMarkerCoordY() { return markerCoordCorrectedY; }

double Vision::getMarkerAngle() { return averagedAngle; }
