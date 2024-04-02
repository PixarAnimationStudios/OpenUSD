

def get_timecode(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid timecode attribute "{usd_attr}"')
        return None
    attr = attr.Get()
    if not attr:
        print(f'{usd_prim} Invalid timecode read attribute "{attr}"')
        return 0.0
    return attr.GetValue()


def get_double(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid double attribute "{usd_attr}"')
        return 0.0
    return attr.Get()

def get_asset(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid asset attribute "{usd_attr}"')
        return None
    attr = attr.Get()
    if not attr:
        print(f'{usd_prim} Invalid read asset attribute "{attr}"')
        return None
    value = attr.path
    return value

def get_string(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid string attribute "{usd_attr}"')
        return None
    return attr.Get()
