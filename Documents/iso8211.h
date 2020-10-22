#ifndef ISO8211_H
#define ISO8211_H

/*
 * This fuke abd associated programs were written by Mike McDonnell
 * of the U.S. Army Topographic Engineering Center (mike@tec.army.mil).
 * They are in the public domain. Please retain this comment.
 */

/* ddr and dr leaders are of a fixed length; 24bytes. */
#define LEADER_LENGTH 24

/*
 * Field and unit terminators are used throughout ISO8211 files. The
 * term "unit" means a subfield within a larger field.
 */

#define FIELD_TERM '\36' /* ctrl-^ */
#define UNIT_TERM '\37'  /* ctrl-_ */

/*
 * These are mnemonic macros showing what the various dda_entry.controls
 * data types are. Besides these numeric values, the trailing chars '&'
 * and/or ';' indicate that these printable chars may be used as
 * printed representations ofUNIT_TERM and FIELD_TERM respectively.
 *
 * The ISO8211 document describes numeric data types as "implicit point"
 * for integers, "explicit point" for floats, and "scaled explicit
 * point" for floats in scientific notation. I have used the more
 * mnemonic names of "INT'', "FLOAT", and "EXP_FLOAT" for these numeric
 * types.
 */

/* The first char is the structure type */
enum structure_type
{
    ELEMENTARY,
    VECTOR,
    ARRAY
};

/* The second char is the basic data type */
enum data_type
{
    CHAR,
    INT,
    FLOAT,
    EXP_FLOAT,
    CHAR_BIT_STRING,
    BITFIELD,
    IGNORE
};
/* label types; make numbers big to stay out of way of lex 's defaults */
enum label_type
{
    VECT = 3,
    CARTESIAN = 4,
    ARRAY_DESC = 5
};

/*
 * The ISO 8211 file consists of a data descriptive record (ddr)
 * followed by data records (dr). This section describes the structures of
 * the ddr. The ddr in tum describes the structures of the dr.
 */

/*
 * The data definition record (ddr) leader is of fixed format; 24 bytes
 * long. I use a standard trick (for me) of defining an ascii struct to
 * overlay the data in the buffer as read and then define a
 * corresponding struct in which ascii elements are appropriately
 * converted.
 */
typedef struct ascii_ddr_leader
{
    char record_length[5];        /* total length of ddr including
                                * terminator */
    char interchange_level[1];    /* 3 levels are defined; l, 2, 3 */
    char leader_id[1];            /* 'L' for ddr leader */
    char extension_flag[1];       /* 'E' for extended char sets; else ' ' */
    char res1[1];                 /* reserved; ' ' for now */
    char application_flag[1];     /* reserved;'' for now */
    char field_control_length[2]; /* bytes in ddf for type and
                                * struct codes; also used in
                                * df? */
    char dda_base[5];             /* offset of dda in ddr */
    char extended[3];             /* specify extended char sets; else '
                                * ' */
    char length_size[1];          /* see below */
    char position_size[1];        /* see below */
    char res2[1];                 /* reserved; '0' for now */
    char tag_size[1];             /* see below */
} ascii_ddr_leader;
typedef struct ddr_leader
{
    int record_length;        /* total length of ddr including
                                * terminator */
    int interchange_level;    /* 3 levels are defined; l, 2, 3 */
    char leader_id[2];        /* 'L' for ddr leader */
    char extension_flag[2];   /* 'E' for extended char sets; else ' ' */
    char res1[2];             /* reserved; ' ' for now */
    char application_flag[2]; /* reserved;'' for now */
    int field_control_length; /* bytes in ddf for type and
                                * struct codes */
    int dda_base;             /* offset of dda in ddr */
    char extended[4];         /* specify extended char sets; else ' ' */
    int length_size;          /* see below */
    int position_size;        /* see below */
    int res2;                 /* reserved; '0' for now */
    int tag_size;             /* see below */
} ddr_leader;

/*
 * Notice that many of these structs have a "next" pointer and so are
 * designed to make lists. As a convention, I do not store the length
 * of these lists. To find length of lists, just traverse them and
 * count the traversals. This is not a very expensive operation and it
 * keeps the data structures simple.
 */

/*
 * A linked list of these structs constitutes the ddr directory. There
 * is a one-to-one correspondence between the ddr_entry structs and the
 * corresponding dda structs as described below. The ddr region is
 * terminated with a FIELD_TERM (ctrl-^).
 *
 * Field tags of 0 and 1 are reserved for the filename and the record ID
 * name respectively. 'length' is the total length of the dda field
 * (see below) including terminator characters. 'position' is the offset
 * of the dda field from the start of the dda area.
 */
typedef struct ddr_entry
{
    char *tag;    /* length gotten from tag_size in leader */
    int length;   /*  ascii length gotten from length_size
                          * in leader */
    int position; /*  ascii length gotten from position_size
                          * in leader */
    struct ddr_entry *next;
} ddr_entry;

/*
 * Vector label tags are separated from each other by a '!' and formats
 * are in parentheses to be able to build up a tree structure as in
 * LISP. Format specification is as in FORTRAN with repeat specs like
 * 4I(7) to specify four integer fields of 7 ascii numeric characters
 * each. See the standard for the (messy) details of the fonnat spec.
 */

typedef struct vector
{ /* needed for cartesian labels more than
                             * 2D */
    char *tag;
    struct vector *next;
} vector;

typedef struct vectors
{
    vector *vec;
    struct vectors *next;
} vectors;

typedef struct cartesian
{
    struct vector *rows;
    struct vector *cols;
    struct vectors *vecs; /* higher dimensions if needed */
} cartesian;

typedef struct array_desc
{
    int length; /* length of a dimension */
    struct array_desc *next;
} array_desc;

typedef union label
{ /* a label will be one of three types */
    struct vector *vector;
    struct cartesian *cartesian;
    struct array_desc *desc;
} label;

/*
 * The format list will be circular at its end since it must
 * automatically repeat within the last set of parens. Rather than
 * actually make the list circular, I define a pointer to the repeating
 * part of the list, which always repeats to the end.
 *
 * An interesting twist in formats is found here in that data may be
 * delimited as well as of a fixed length. Thus A(,) means a string of
 * ASCII characters delimited by a comma. Data may be either delimited
 * or have a fixed length. Therefore at least one of the members
 * "length" or "delimiter" must be zero. They may also both be zero for
 * data delimited by UNIT_TERM.
 */
typedef struct format
{
    int type;   /* INT, FLOAT, EXP _FLOAT, ... */
    int length; /* either this or delimiter must be 00 */
    char delimiter;
    struct format *next;
} format;

typedef struct ascii_dda_entry
{
    char *controls; /* length is gotten from header
                                 * field_control_length */
    char *name;     /* length up to terminator */
    char *label;    /* length up to terminator */
    char *format;   /* length up to terminator */
    struct ascii_dda_entry *next;
} ascii_dda_entry;

typedef struct dda_entry
{
    int structure_type; /* ELEMENTARY, VECTOR, ARRAY */
    int data_type;      /* INT, FLOAT, EXP _FLOAT, ... */
    char *name;         /* long descriptive name */
    char *tag;          /* same as in corresponding ddr_entry */
    int label_type;     /* VECT, CARI'ESIAN, ARRAY_DESC */
    union label *label;
    struct format *format;
    struct format *repeat; /* indicate repeating part of fonnat
                                 * list */
    struct dda_entry *next;
} dda_entry;

/*
 * The ISO 8211 file consists of a data descriptive record (ddr)
 * followed by data records (dr). This section describes the basic
 * structures of all dr. The ddr describes the structures of
 * each dr region. See above for data structures of the ddr.
 *
 * The data record (dr) leader is of fixed format; 24 bytes long.
 *
 * Standard trick here; make an all-ascii struct to overlay on the input
 * buffer and pick up the fields, then have another struct with the same
 * field names which are now integers, etc as appropriate. Note that
 * even single-character fields are saved as strings so that strncmp()
 * may be used consistently for all comparisons.
 */

typedef struct ascii_dr_leader
{
    char record_length[5]; /* total length of ddr including
                                * terminator */
    char res1[1];          /* reserved; ' ' for now */
    char leader_id[1];     /* 'D' for once; 'R' ddr repeat */
    char res2[5];          /* reserved; 5 spaces ' ' for now */
    char data_base[5];     /* offset of data area (uda) in dr */
    char res3[3];          /* reserved; 3 spaces ' ' for now */
    char length_size[1];   /* see below */
    char position_size[1]; /* see below */
    char res4[1];          /* reserved; '0' for now */
    char tag_size[1];      /* see below */
} ascii_dr_leader;

typedef struct dr_leader
{
    int record_length; /* total length of dr */
    char res1[2];      /* reserved; ' ' for now */
    char leader_id[2]; /* 'D' for once; 'R' ddr repeat */
    char res2[6];      /* reserved; 5 spaces ' ' for now */
    int data_base;     /* offset of data area (uda) in dr */
    char res3[4];      /* reserved; 3 spaces ' ' for now */
    int length_size;   /* see below */
    int position_size; /* see below */
    int res4;          /* reserved; '0' for now */
    int tag_size;      /* see below */
} dr_leader;

/*
 * A linked list of these structs constirutes the dr directory. There is
 * a correspondence between the dr_entry structs and the
 * uda (user data area) structs as described below.
 * Corresponding structs are matched by the "key" member in dr_entry
 * and the "field_tag" member in uda_entry. The
 * directory region is tenninated with a FIELD_TERM.
 *
 * 'length' is the total length of the uda field (see below) including
 * tenninator characters. 'position' is the offset of the uda field from
 * the start of the uda area.
 *
 * This is exactly the same as a ddr_entry struct I may combine them
 * some day, I just didn't realize that they were the same until I was
 * done with the parser. Keeping them separate makes it easier to
 * keep the names of things separate anyway.
 */
typedef struct dr_entry
{
    char *tag;    /* length gotten from tag_size in leader */
    int length;   /* length of "length" is gotten from
                         * length_size in leader*/
    int position; /* length is gotten from position_size
                         * in leader*/
    struct dr_entry *next;
} dr_entry;

/*
 * This is the user data area (uda) of the dr.
 * The length of the uda list will be the same as the length of the
 * dr_entry list above. Each entry in the uda is also tenninated
 * with a FIELD_TERM (ctrl-^).
 *
 * The only thing that might have to be
 * handled specially in here is if arrays are defined by an array
 * descriptor in the uda; a strange beast that is just like an array
 * descriptor as may be found in the dda label field except that it
 * has its fields separated by UNIT_TERM (ctrl-_) instead of commas.
 */
typedef struct uda_entry
{
    char *field_tag; /* length is up to field terminator */
    char *vec_tag;   /* length is up to next vector item */
    char *type;      /* A,I,R,S,C,B, or X */
    union
    {
        char *cp;     /* CHAR (acrually a string) */
        int i;        /* INT */
        double d;     /* FLOAT, EXP_FLOAT */
        int *bf;      /* BITFIELD, CHAR_BIT_STRING */
        void *ignore; /* IGNORE */
    } data;           /* user data. */
    struct uda_entry *next;
} uda_entry;

extern char *malloc();     /* Have to put this somewhere. */
extern format *formatlist; /* global pointer where format list goes */
extern format *repeatlist; /* global pointer for format list repeat*/

/*
 * All the public functions
 */
extern struct ddr_leader *parse_ddr_leader();
extern struct ddr_entry *parse_ddr_directory();
extern struct dda_entry *parse_dda();
extern struct dda_entry *parse_ddr();
extern struct dr_leader *parse_dr_leader();
extern struct dr_entry *parse_dr_directory();
extern struct dr_entry *parse_dr();

#endif /* ISO8211_H */