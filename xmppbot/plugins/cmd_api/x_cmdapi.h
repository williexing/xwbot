/*
 * cmdapi.h
 *
 *  Created on: Nov 4, 2011
 *      Author: artemka
 */

#ifndef CMDAPI_H_
#define CMDAPI_H_

#include <xwlib++/x_objxx.h>

#include <plugins/sessions_api/sessions.h>

class xcmdapi : public gobee::__x_object
{
    //    x_common_session *iosession;
    char *sid;
    struct
    {
        int state;
        u_int cr0;
    } regs;

    typedef enum
    {
        NOACTION = 0,
        EXECUTE = 1,
        COMPLETE,
        CANCEL,
        NEXT,
        PREV
    }ACTION;

public:
    xcmdapi();
    ACTION get_action();
    virtual int equals(x_obj_attr_t *);
    virtual void on_create();
    virtual int on_append(__x_object *parent);
    virtual void on_remove();
    virtual void on_release();                  // on_release
    virtual __x_object *operator+=(x_obj_attr_t *);
    virtual void commit();
    virtual int classmatch(x_obj_attr_t *);

public:
    gobee::__x_object *get_response(gobee::__x_object *req, ACTION);
    const static char *cmdName;
    const static char *cmdXmlns;

private:
    gobee::__x_object *onExecute(gobee::__x_object *req);
    gobee::__x_object *onNext(gobee::__x_object *req);
    void CallBack(const char *jid);
};

#endif /* CMDAPI_H_ */
