"""Numeric interpretations of the per-item Decor Art Briefs.

Periods inside authored ranges vary deterministically per copy. Unspecified
secondary periods use the primary loop; these affect presentation only.
"""
def animation_for(item_id):
    # primary motion, degrees, seconds, local brightness fraction, light seconds
    rows = {
        'PP-01': ('sway',2,7,.12,9), 'PP-02': ('sway',3,8,0,0),
        'PP-03': ('nod',2,6,.10,10), 'PP-04': ('open',6,21,0,0),
        'PP-05': ('sway',2,9,.08,14), 'PP-06': ('pulse',0,11,.15,11),
        'PP-07': ('sway',2,8,.08,14), 'PP-08': ('nod',3,7,0,0),
        'PP-09': ('sway',1.5,10,.10,14), 'PP-10': ('open',5,16,0,0),
        'PP-11': ('open',4,22,.08,22), 'PP-12': ('sway',1,12,.08,18),
        'PP-13': ('sway',1.5,12,.08,16), 'PP-14': ('open',2,12,0,0),
        'PP-15': ('open',4,16,.10,24), 'PP-16': ('sway',1,14,.08,20),
        'PD-01': ('clam',20,21,0,0), 'PD-02': ('pulse',0,9,.15,9),
        'PD-03': ('bubble',0,17,0,0), 'PD-04': ('pincer',8,22,0,0),
        'PD-05': ('chime',2,9.5,.08,14), 'PD-06': ('sail',3,7,0,0),
        'PD-07': ('carousel',2,10,0,0), 'PD-08': ('bubble',0,30,.12,12),
        'PD-09': ('pulse',0,15,.12,15), 'PD-10': ('lid',10,27,0,0),
        'PD-11': ('chime',2,10,.08,14), 'PD-12': ('internalBubble',0,14,0,0),
        'PD-13': ('lid',12,26,.10,26), 'PD-14': ('ring',4,18,.08,18),
        'PD-15': ('chime',1.5,16,.10,16), 'PD-16': ('lamp',0,18,.10,18),
        'PL-01': ('chime',2,10,.10,16), 'PL-02': ('nod',2,11,.08,18),
        'PL-03': ('bucket',3,24,.10,24), 'PL-04': ('sway',1.5,12,0,0),
        'PL-05': ('bubble',2,22,0,0), 'PL-06': ('sway',2,9,.10,15),
        'PL-07': ('pinwheel',360,18,0,0), 'PL-08': ('open',5,14,.08,14),
        'PL-09': ('chime',2,10,.08,16), 'PL-10': ('nod',2,10,.12,17),
        'PL-11': ('snowglobe',0,12,0,0), 'PL-12': ('open',4,20,.10,20),
    }
    if item_id not in rows:
        return {'kind': 'static'}
    kind, degrees, seconds, light, light_seconds = rows[item_id]
    result = dict(kind=kind, degrees=degrees, seconds=seconds, brightness=light,
                  light_seconds=light_seconds, source='Decor Art Briefs: '+item_id)
    ranges={'PP-04':(18,24),'PP-07':(12,16),'PD-01':(18,24),'PD-03':(14,20),
            'PD-04':(18,26),'PD-08':(25,35),'PD-10':(24,30),'PD-13':(22,30),
            'PL-03':(20,28),'PL-05':(18,26)}
    if item_id in ranges: result['light_period_range' if item_id=='PP-07' else 'period_range']=list(ranges[item_id])
    holds={'PP-04':2,'PP-11':3,'PP-15':2,'PD-01':2,'PD-10':3,'PL-03':4,'PL-12':3}
    result['hold_seconds']=holds.get(item_id,2)
    if kind=='bubble':
        result.update(particle_max=2 if item_id=='PD-03' else 1,
                      travel_units=.7 if item_id=='PD-03' else .5)
    if kind=='internalBubble': result.update(particle_max=2,travel_units=.4)
    if kind=='snowglobe': result.update(particle_max=3,travel_units=.15)
    return result
