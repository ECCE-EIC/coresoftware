#ifndef TRACKRECO_PHACTSSILICONSEEDING_H
#define TRACKRECO_PHACTSSILICONSEEDING_H

#include <trackbase/ActsTrackingGeometry.h>

#include <fun4all/SubsysReco.h>
#include <trackbase/TrkrDefs.h>
#include <trackbase/ActsSurfaceMaps.h>

#include <Acts/Utilities/Definitions.hpp>
#include <Acts/Geometry/GeometryIdentifier.hpp>
#include <Acts/Seeding/Seedfinder.hpp>
#include <Acts/Utilities/Units.hpp>

#include <Acts/Seeding/BinFinder.hpp>
#include <Acts/Seeding/SpacePointGrid.hpp>

#include <ActsExamples/EventData/TrkrClusterSourceLink.hpp>

#include <boost/bimap.hpp>

#include <string>
#include <map>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>

class PHCompositeNode;
class PHG4CylinderGeomContainer;
class SvtxTrackMap;
class SvtxVertexMap;
class TrkrCluster;
class TrkrClusterContainer;
class TrkrHitSetContainer;
class TrkrClusterIterationMapv1;

using SourceLink = ActsExamples::TrkrClusterSourceLink;
typedef boost::bimap<TrkrDefs::cluskey, unsigned int> CluskeyBimap;

/**
 * A struct for Acts to take cluster information for seeding
 */
struct SpacePoint {
  TrkrDefs::cluskey m_clusKey;
  float m_x;
  float m_y;
  float m_z;
  float m_r;
  Acts::GeometryIdentifier m_geoId;
  float m_varianceRphi;
  float m_varianceZ;
  
  TrkrDefs::cluskey Id() const { return m_clusKey; }

  /// These are needed by Acts
  float x() const { return m_x; }
  float y() const { return m_y; }
  float z() const { return m_z; }
  float r() const { return m_r; }

};

/// This is needed by the Acts seedfinder 
inline bool operator==(SpacePoint a, SpacePoint b) {
  return (a.m_clusKey == b.m_clusKey);
}

using SpacePointPtr = std::unique_ptr<SpacePoint>;
using GridSeeds = std::vector<std::vector<Acts::Seed<SpacePoint>>>;

/**
 * This class runs the Acts seeder over the MVTX measurements
 * to create track stubs for the rest of the stub matching pattern
 * recognition. The module also projects the MVTX stubs to the INTT
 * to find possible matches in the INTT to the MVTX triplet.
 */
class PHActsSiliconSeeding : public SubsysReco
{
 public:
  PHActsSiliconSeeding(const std::string& name = "PHActsSiliconSeeding");
  int Init(PHCompositeNode *topNode) override;
  int InitRun(PHCompositeNode *topNode) override;
  int process_event(PHCompositeNode *topNode) override;
  int End(PHCompositeNode *topNode) override;

  /// Set seeding with truth clusters
  void useTruthClusters(bool useTruthClusters)
    { m_useTruthClusters = useTruthClusters; }

  /// Output some diagnostic histograms
  void seedAnalysis(bool seedAnalysis)
    { m_seedAnalysis = seedAnalysis; }

  /// field map name for 3d map functionality
  void fieldMapName(const std::string& fieldmap)
    { m_fieldMapName = fieldmap; }

  /// For each MVTX+INTT seed, take the best INTT hits and form
  /// 1 silicon seed per MVTX seed
  void cleanSeeds(bool cleanSeeds)
    { m_cleanSeeds = cleanSeeds;}
 
  void rMax(const float rMax)
  { m_rMax = rMax; }
  void rMin(const float rMin)
  { m_rMin = rMin; }
  void zMax(const float zMax)
  {m_zMax = zMax; }
  void zMin(const float zMin)
  {m_zMin = zMin; }
  void deltaRMax(const float deltaRMax)
  {m_deltaRMax = deltaRMax;}
  void cotThetaMax(const float cotThetaMax)
  {m_cotThetaMax = cotThetaMax;}
  void gridFactor(const float gridFactor)
  {m_gridFactor = gridFactor;}

  /// A function to run the seeder with large (true)
  /// or small (false) grid spacing
  void largeGridSpacing(const bool spacing);

  void set_track_map_name(const std::string &map_name) { _track_map_name = map_name; }
  void SetIteration(int iter){_n_iteration = iter;}

 private:

  int getNodes(PHCompositeNode *topNode);
  int createNodes(PHCompositeNode *topNode);

  GridSeeds runSeeder(std::vector<const SpacePoint*>& spVec);

  /// Configure the seeding parameters for Acts. There
  /// are a number of tunable parameters for the seeder here
  Acts::SeedfinderConfig<SpacePoint> configureSeeder();
  Acts::SpacePointGridConfig configureSPGrid();
  
  /// Take final seeds and fill the SvtxTrackMap
  void makeSvtxTracks(GridSeeds& seedVector);
  
  /// Create a seeding space point out of an Acts::SourceLink
  SpacePointPtr makeSpacePoint(const TrkrDefs::cluskey cluskey, 
			       const SourceLink& sl);
  
  /// Get all space points for the seeder
  std::vector<const SpacePoint*> getMvtxSpacePoints();

  /// Perform circle/line fits with the final MVTX seed to get
  /// initial point and momentum estimates for stub matching
  int circleFitSeed(std::vector<TrkrCluster*>& clusters,
		    std::vector<Acts::Vector3D>& clusGlobPos,
		    double& x, double& y, double& z,
		    double& px, double& py, double& pz);

  void circleFitByTaubin(const std::vector<Acts::Vector3D>& globalPositions,
			 double& R, double& X0, double& Y0);
  void lineFit(const std::vector<Acts::Vector3D>& globPos,
	       double& A, double& B);
  void findRoot(const double R, const double X0, const double Y0,
		double& x, double& y);
  int getCharge(const std::vector<Acts::Vector3D>& globalPos,
		const double circPhi);

  /// Projects circle fit to INTT radii to find possible INTT clusters
  /// belonging to MVTX track stub
  std::vector<TrkrDefs::cluskey> findInttMatches(
		        std::vector<Acts::Vector3D>& clusters,
			const double R,
			const double X0,
			const double Y0,
			const double B,
			const double m);

  std::vector<TrkrDefs::cluskey> matchInttClusters(std::vector<Acts::Vector3D>& clusters,
						   const double xProj[],
						   const double yProj[],
						   const double zProj[]);
  void circleCircleIntersection(const double layerRadius, 
				const double circRadius,
				const double circX0,
				const double circY0,
				double& xplus,
				double& yplus,
				double& xminus,
				double& yminus);

  void createSvtxTrack(const double x,
		       const double y,
		       const double z,
		       const double px,
		       const double py,
		       const double pz,
		       const int charge,
		       std::vector<TrkrCluster*>& clusters,
		       std::vector<Acts::Vector3D>& clusGlobPos);
  std::map<const unsigned int, std::pair<std::vector<TrkrCluster*>,std::vector<Acts::Vector3D>>>
    makePossibleStubs(std::vector<TrkrCluster*>& allClusters,
		      std::vector<Acts::Vector3D>& clusGlobPos);

  Surface getSurface(TrkrDefs::hitsetkey hitsetkey);

  std::map<const unsigned int, std::pair<std::vector<TrkrCluster*>,std::vector<Acts::Vector3D>>>
    identifyBestSeed(std::map<const unsigned int, 
		     std::pair<std::vector<TrkrCluster*>,
		               std::vector<Acts::Vector3D>>>);

  void createHistograms();
  void writeHistograms();
  double normPhi2Pi(const double phi);

  ActsTrackingGeometry *m_tGeometry = nullptr;
  SvtxTrackMap *m_trackMap = nullptr;
  TrkrClusterContainer *m_clusterMap = nullptr;
  TrkrHitSetContainer  *m_hitsets = nullptr;
  PHG4CylinderGeomContainer *m_geomContainerIntt = nullptr;
  ActsSurfaceMaps *m_surfMaps = nullptr;
  
  /// Configuration classes for Acts seeding
  Acts::SeedfinderConfig<SpacePoint> m_seedFinderCfg;
  Acts::SpacePointGridConfig m_gridCfg;

  /// Configurable parameters
  /// seed pt has to be in MeV
  float m_minSeedPt = 100;

  /// How many seeds a given hit can be the middle hit of the seed
  /// MVTX can only have the middle layer be the middle hit
  int m_maxSeedsPerSpM = 1;

  /// Limiting location of measurements (e.g. detector constraints)
  /// We limit to the MVTX
  float m_rMax = 200.;
  float m_rMin = 23.;
  float m_zMax = 300.;
  float m_zMin = -300.;

  /// Value tuned to provide as large of phi bins as possible. 
  /// Increases the secondary finding efficiency
  float m_gridFactor = 2.3809;

  /// max distance between two measurements in one seed
  float m_deltaRMax = 15;
  
  /// Cot of maximum theta angle
  float m_cotThetaMax = 2.9;
  
  /// B field value in z direction
  /// bfield for space point grid neds to be in kiloTesla
  float m_bField = 1.4 / 1000.;

  std::shared_ptr<Acts::BinFinder<SpacePoint>> 
    m_bottomBinFinder, m_topBinFinder;

  int m_event = 0;

  /// Maximum allowed transverse PCA for seed, cm
  double m_maxSeedPCA = 2.;
  
  const static unsigned int m_nInttLayers = 4;
  const double m_nInttLayerRadii[m_nInttLayers] = 
    {7.188, 7.732, 9.680,10.262}; /// cm
  
  /// Search window for phi to match intt clusters in cm
  double m_rPhiSearchWin = 0.1;

  /// Whether or not to use truth clusters in hit lookup
  bool m_useTruthClusters = false;

  bool m_cleanSeeds = false;

  int m_nBadUpdates = 0;
  int m_nBadInitialFits = 0;
  std::string m_fieldMapName = "";
  TrkrClusterIterationMapv1* _iteration_map = nullptr;
  int _n_iteration = 0;
  std::string _track_map_name = "SvtxSiliconTrackMap";

  bool m_seedAnalysis = false;
  TFile *m_file = nullptr;
  TH2 *h_nInttProj = nullptr;
  TH1 *h_nMvtxHits = nullptr;
  TH1 *h_nInttHits = nullptr;
  TH2 *h_nHits = nullptr;
  TH1 *h_nSeeds = nullptr;
  TH1 *h_nActsSeeds = nullptr;
  TH1 *h_nTotSeeds = nullptr;
  TH1 *h_nInputMeas = nullptr;
  TH1 *h_nInputMvtxMeas = nullptr;
  TH1 *h_nInputInttMeas = nullptr;
  TH2 *h_hits = nullptr;
  TH2 *h_zhits = nullptr;
  TH2 *h_projHits = nullptr;
  TH2 *h_zprojHits = nullptr;
  TH2 *h_resids = nullptr;

};


#endif
