#include "IGTLinkClinet.h"
#include "ui_IGTLinkClinet.h"

class IGTLinkClinetPrivate : public Ui_IGTLinkClinet
{
	Q_DECLARE_PUBLIC(IGTLinkClinet);
protected:
	IGTLinkClinet* const q_ptr;

public:

	IGTLinkClinetPrivate(IGTLinkClinet& object);

};

//-----------------------------------------------------------------------
IGTLinkClinetPrivate::IGTLinkClinetPrivate(IGTLinkClinet& object)
	: q_ptr(&object)
{
}
//-----------------------------------------------------------------------
IGTLinkClinet::IGTLinkClinet(QWidget* parent)
	: d_ptr(new IGTLinkClinetPrivate(*this))
{
	Q_D(IGTLinkClinet);
	d->setupUi(this);
}

IGTLinkClinet::~IGTLinkClinet()
{

}
