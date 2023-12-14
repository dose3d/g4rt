/**
*
* \author B.Rachwal (brachwal@agh.edu.pl), J.Hajduga
* \date 10.08.2020
*
*/

#ifndef Dose3D_DicomSvcSVC_H
#define Dose3D_DicomSvcSVC_H

#include "Types.hh"

////////////////////////////////////////////////////////////////////////////////
///
class DicomSvc {
  private:
    /// \brief The full path the RT-Plan file
    std::string m_rtplan_file;

    /// \brief Current beam number
    int m_beamId = 0; // temporary fixed

    /// \brief Current control point number
    int m_controlPointId = 0; // temporary fixed

    ///
    int m_rt_plan_n_beams = 0;
    std::vector<int> m_rtplan_beam_n_control_points;

    ///
    DicomSvc();

    ///
    ~DicomSvc() = default;

    /// Delete the copy and move constructors
    DicomSvc(const DicomSvc &) = delete;
    DicomSvc &operator=(const DicomSvc &) = delete;
    DicomSvc(DicomSvc &&) = delete;
    DicomSvc &operator=(DicomSvc &&) = delete;

    ///
    void Initialize();

  public:
    ///\brief Static method to get instance of this singleton object.
    static DicomSvc* GetInstance();

    ///\brief  Mądry opis
    G4double GetRTPlanJawPossition(const std::string& jawName, int current_beam, int current_controlpoint) const;

    ///
    std::vector<G4double> GetRTPlanMlcPossitioning(const std::string& side, int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    double GetRTPlanAngle(int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    double GetRTPlanDose(int current_beam, int current_controlpoint) const;

    ///\brief  Describe me.
    unsigned GetRTPlanNumberOfBeams() const;

    ///\brief  Sum of the control points for a given beam number
    unsigned GetRTPlanNumberOfControlPoints(unsigned beamNumber) const;

    ///
    unsigned GetTotalNumberOfControlPoints() const;
};
#endif  // Dose3D_DicomSvcSVC_H
