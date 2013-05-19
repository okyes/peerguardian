
#include "blocklist.h"
#include "blocklist_p.h"

#include <QFile>

Blocklist::Blocklist(const QString& text, bool active, bool enabled) :
    Option(),
    d_active_ptr(new BlocklistPrivate),
    d_ptr(new BlocklistPrivate)
{
    setActiveData(d_active_ptr);
    setData(d_ptr);

    d_ptr->local = false;
    if (QFile::exists(text))
        d_ptr->local = true;

    setEnabled(enabled);
    setName(parseName(text));
    setValue(parseUrl(text));
    d_ptr->valid = isValid(text);
    d_ptr->removed = false;

    if (active)
        *d_active_ptr = *d_ptr;
}

Blocklist::Blocklist(const Blocklist& blocklist)
{
    *this = blocklist;
}

Blocklist::~Blocklist()
{
    //will be deleted by Option destructor
    d_ptr = 0;
    d_active_ptr = 0;
}

bool Blocklist::isLocal() const
{
    return d_ptr->local;
}

bool Blocklist::isValid() const
{
    return d_ptr->valid;
}

QString Blocklist::location() const
{
    return d_ptr->value.toString();
}

bool Blocklist::exists() const
{
    if (isLocal())
        return QFile::exists(location());
    return true; //if it's a remote blocklist, assume it exists
}

bool Blocklist::isValid(const QString & url)
{
    //QStringList formats;
    //formats << "7z" << "dat" << "gz" << "p2p" << "zip" << "txt" << "";

    if ( url.contains("list.iblocklist.com") )
        return true;

    if ( url.contains("http://") || url.contains("ftp://") || url.contains("https://") )
        return true;

    if ( QFile::exists(url) )
        return true;
    //foreach(const QString& format, formats)
    //    if ( line.endsWith(format) )
    //        return true;

    return false;
}

QString Blocklist::parseName(const QString& text)
{
    if ( text.contains("list.iblocklist.com/lists/") )
        return text.split("list.iblocklist.com/lists/").last();

    if ( text.contains("http://") || text.contains("ftp://") || text.contains("https://") )
        return text.split("/").last();

    QFileInfo file(text);

    if ( file.exists() )
        return file.fileName();

    return text;
}

QString Blocklist::parseUrl(const QString& text)
{
    if (text.contains('#') && text.split('#').size() >= 2)
        return text.split("#")[1];

    return text;
}

