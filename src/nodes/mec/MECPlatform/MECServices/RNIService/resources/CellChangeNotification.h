//
//      CellChangeNotification.h -  Implementation of CellChangeNotification 
//
// Author: Angelo Feraudo (University of Bologna)
//

#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_CELLCHANGENOTIFICATION_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_CELLCHANGENOTIFICATION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/NotificationBase.h"
#include "FilterCriteriaAssocHo.h"

class CellChangeNotification : public NotificationBase{
public:
    CellChangeNotification();
    virtual ~CellChangeNotification();
    virtual nlohmann::ordered_json toJson() const override;
    virtual bool fromJson(const nlohmann::ordered_json& json) override;
    virtual EventNotification* handleNotification(FilterCriteriaBase *filters, bool noCheck=false) override;

    void setHoStatus(HoStatus hoStatus) {hoStatus_ = hoStatus;};
    void setSrcEcgi(Ecgi srcEcgi) {srcEcgi_ = srcEcgi;};
    void setTrgEcgi(Ecgi trgEcgi) {trgEcgi_ = trgEcgi;};

    HoStatus getHoStatus() const {return hoStatus_;}
    Ecgi getSrcEcgi() const {return srcEcgi_;}
    Ecgi getTrgEcgi() const {return trgEcgi_;}
    
private:
    HoStatus hoStatus_;
    Ecgi srcEcgi_;
    Ecgi trgEcgi_;
    //missing tempUeId, however it is optional

};


#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_CELLCHANGENOTIFICATION_H_ */
