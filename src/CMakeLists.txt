cmake_minimum_required (VERSION 3.18)
project (HDF5_SRC C)

#-----------------------------------------------------------------------------
# List Source Files
#-----------------------------------------------------------------------------
set (H5_SOURCES
    ${HDF5_SRC_DIR}/H5.c
    #${HDF5_SRC_BINARY_DIR}/H5build_settings.c
    ${HDF5_SRC_DIR}/H5checksum.c
    ${HDF5_SRC_DIR}/H5dbg.c
    ${HDF5_SRC_DIR}/H5mpi.c
    ${HDF5_SRC_DIR}/H5system.c
    ${HDF5_SRC_DIR}/H5timer.c
    ${HDF5_SRC_DIR}/H5trace.c
)

set (H5_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/hdf5.h
    ${HDF5_SRC_DIR}/H5api_adpt.h
    ${HDF5_SRC_DIR}/H5public.h
    #${HDF5_SRC_DIR}/H5version.h
    #${HDF5_SRC_DIR}/H5overflow.h
)
IDE_GENERATED_PROPERTIES ("H5" "${H5_HDRS}" "${H5_SOURCES}" )

set (H5_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5encode.h
    ${HDF5_SRC_DIR}/H5private.h
    ${HDF5_SRC_DIR}/H5timer.h
    ${HDF5_SRC_DIR}/H5warnings.h
    ${HDF5_SRC_DIR}/H5win32defs.h
    ${HDF5_SRC_DIR}/uthash.h
)


set (H5A_SOURCES
    ${HDF5_SRC_DIR}/H5A.c
    ${HDF5_SRC_DIR}/H5Abtree2.c
    ${HDF5_SRC_DIR}/H5Adense.c
    ${HDF5_SRC_DIR}/H5Adeprec.c
    ${HDF5_SRC_DIR}/H5Aint.c
    ${HDF5_SRC_DIR}/H5Atest.c
)

set (H5A_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Apublic.h
)
IDE_GENERATED_PROPERTIES ("H5A" "${H5A_HDRS}" "${H5A_SOURCES}" )

set (H5A_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Apkg.h
    ${HDF5_SRC_DIR}/H5Aprivate.h
)


set (H5AC_SOURCES
    ${HDF5_SRC_DIR}/H5AC.c
    ${HDF5_SRC_DIR}/H5ACdbg.c
    ${HDF5_SRC_DIR}/H5ACmpio.c
    ${HDF5_SRC_DIR}/H5ACproxy_entry.c
)

set (H5AC_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5ACpublic.h
)
IDE_GENERATED_PROPERTIES ("H5AC" "${H5AC_HDRS}" "${H5AC_SOURCES}" )

set (H5AC_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5ACpkg.h
    ${HDF5_SRC_DIR}/H5ACprivate.h
)


set (H5B_SOURCES
    ${HDF5_SRC_DIR}/H5B.c
    ${HDF5_SRC_DIR}/H5Bcache.c
    ${HDF5_SRC_DIR}/H5Bdbg.c
)
set (H5B_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5B" "${H5B_HDRS}" "${H5B_SOURCES}" )

set (H5B_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Bpkg.h
    ${HDF5_SRC_DIR}/H5Bprivate.h
)


set (H5B2_SOURCES
    ${HDF5_SRC_DIR}/H5B2.c
    ${HDF5_SRC_DIR}/H5B2cache.c
    ${HDF5_SRC_DIR}/H5B2dbg.c
    ${HDF5_SRC_DIR}/H5B2hdr.c
    ${HDF5_SRC_DIR}/H5B2int.c
    ${HDF5_SRC_DIR}/H5B2internal.c
    ${HDF5_SRC_DIR}/H5B2leaf.c
    ${HDF5_SRC_DIR}/H5B2stat.c
    ${HDF5_SRC_DIR}/H5B2test.c
)
set (H5B2_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5B2" "${H5B2_HDRS}" "${H5B2_SOURCES}" )

set (H5B2_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5B2pkg.h
    ${HDF5_SRC_DIR}/H5B2private.h
)


set (H5C_SOURCES
    ${HDF5_SRC_DIR}/H5C.c
    ${HDF5_SRC_DIR}/H5Cdbg.c
    ${HDF5_SRC_DIR}/H5Centry.c
    ${HDF5_SRC_DIR}/H5Cepoch.c
    ${HDF5_SRC_DIR}/H5Cimage.c
    ${HDF5_SRC_DIR}/H5Cint.c
    ${HDF5_SRC_DIR}/H5Clog.c
    ${HDF5_SRC_DIR}/H5Clog_json.c
    ${HDF5_SRC_DIR}/H5Clog_trace.c
    ${HDF5_SRC_DIR}/H5Cmpio.c
    ${HDF5_SRC_DIR}/H5Cprefetched.c
    ${HDF5_SRC_DIR}/H5Cquery.c
    ${HDF5_SRC_DIR}/H5Ctag.c
    ${HDF5_SRC_DIR}/H5Ctest.c
)
set (H5C_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Cpublic.h
)
IDE_GENERATED_PROPERTIES ("H5C" "${H5C_HDRS}" "${H5C_SOURCES}" )

set (H5C_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Clog.h
    ${HDF5_SRC_DIR}/H5Cpkg.h
    ${HDF5_SRC_DIR}/H5Cprivate.h
)


set (H5CX_SOURCES
    ${HDF5_SRC_DIR}/H5CX.c
)
set (H5CX_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5CX" "${H5CX_HDRS}" "${H5CX_SOURCES}" )

set (H5CX_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5CXprivate.h
)


set (H5D_SOURCES
    ${HDF5_SRC_DIR}/H5D.c
    ${HDF5_SRC_DIR}/H5Dbtree.c
    ${HDF5_SRC_DIR}/H5Dbtree2.c
    ${HDF5_SRC_DIR}/H5Dchunk.c
    ${HDF5_SRC_DIR}/H5Dcompact.c
    ${HDF5_SRC_DIR}/H5Dcontig.c
    ${HDF5_SRC_DIR}/H5Ddbg.c
    ${HDF5_SRC_DIR}/H5Ddeprec.c
    ${HDF5_SRC_DIR}/H5Dearray.c
    ${HDF5_SRC_DIR}/H5Defl.c
    ${HDF5_SRC_DIR}/H5Dfarray.c
    ${HDF5_SRC_DIR}/H5Dfill.c
    ${HDF5_SRC_DIR}/H5Dint.c
    ${HDF5_SRC_DIR}/H5Dio.c
    ${HDF5_SRC_DIR}/H5Dlayout.c
    ${HDF5_SRC_DIR}/H5Dmpio.c
    ${HDF5_SRC_DIR}/H5Dnone.c
    ${HDF5_SRC_DIR}/H5Doh.c
    ${HDF5_SRC_DIR}/H5Dscatgath.c
    ${HDF5_SRC_DIR}/H5Dselect.c
    ${HDF5_SRC_DIR}/H5Dsingle.c
    ${HDF5_SRC_DIR}/H5Dtest.c
    ${HDF5_SRC_DIR}/H5Dvirtual.c
)

set (H5D_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Dpublic.h
)
IDE_GENERATED_PROPERTIES ("H5D" "${H5D_HDRS}" "${H5D_SOURCES}" )

set (H5D_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Dpkg.h
    ${HDF5_SRC_DIR}/H5Dprivate.h
)


set (H5E_SOURCES
    ${HDF5_SRC_DIR}/H5E.c
    ${HDF5_SRC_DIR}/H5Edeprec.c
    ${HDF5_SRC_DIR}/H5Eint.c
)

set (H5E_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Epubgen.h
    ${HDF5_SRC_DIR}/H5Epublic.h
)
IDE_GENERATED_PROPERTIES ("H5E" "${H5E_HDRS}" "${H5E_SOURCES}" )

set (H5E_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Edefin.h
    ${HDF5_SRC_DIR}/H5Einit.h
    ${HDF5_SRC_DIR}/H5Emajdef.h
    ${HDF5_SRC_DIR}/H5Emindef.h
    ${HDF5_SRC_DIR}/H5Epkg.h
    ${HDF5_SRC_DIR}/H5Eprivate.h
    ${HDF5_SRC_DIR}/H5Eterm.h
)


set (H5EA_SOURCES
    ${HDF5_SRC_DIR}/H5EA.c
    ${HDF5_SRC_DIR}/H5EAcache.c
    ${HDF5_SRC_DIR}/H5EAdbg.c
    ${HDF5_SRC_DIR}/H5EAdblkpage.c
    ${HDF5_SRC_DIR}/H5EAdblock.c
    ${HDF5_SRC_DIR}/H5EAhdr.c
    ${HDF5_SRC_DIR}/H5EAiblock.c
    ${HDF5_SRC_DIR}/H5EAint.c
    ${HDF5_SRC_DIR}/H5EAsblock.c
    ${HDF5_SRC_DIR}/H5EAstat.c
    ${HDF5_SRC_DIR}/H5EAtest.c
)
set (H5EA_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5EA" "${H5EA_HDRS}" "${H5EA_SOURCES}" )

set (H5EA_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5EApkg.h
    ${HDF5_SRC_DIR}/H5EAprivate.h
)


set (H5ES_SOURCES
    ${HDF5_SRC_DIR}/H5ES.c
    ${HDF5_SRC_DIR}/H5ESevent.c
    ${HDF5_SRC_DIR}/H5ESint.c
    ${HDF5_SRC_DIR}/H5ESlist.c
)
set (H5ES_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5ESdevelop.h
    ${HDF5_SRC_DIR}/H5ESpublic.h
)
IDE_GENERATED_PROPERTIES ("H5ES" "${H5ES_HDRS}" "${H5ES_SOURCES}" )

set (H5ES_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5ESpkg.h
    ${HDF5_SRC_DIR}/H5ESprivate.h
)


set (H5F_SOURCES
    ${HDF5_SRC_DIR}/H5F.c
    ${HDF5_SRC_DIR}/H5Faccum.c
    ${HDF5_SRC_DIR}/H5Fcwfs.c
    ${HDF5_SRC_DIR}/H5Fdbg.c
    ${HDF5_SRC_DIR}/H5Fdeprec.c
    ${HDF5_SRC_DIR}/H5Fefc.c
    ${HDF5_SRC_DIR}/H5Ffake.c
    ${HDF5_SRC_DIR}/H5Fint.c
    ${HDF5_SRC_DIR}/H5Fio.c
    ${HDF5_SRC_DIR}/H5Fmount.c
    ${HDF5_SRC_DIR}/H5Fmpi.c
    ${HDF5_SRC_DIR}/H5Fquery.c
    ${HDF5_SRC_DIR}/H5Fsfile.c
    ${HDF5_SRC_DIR}/H5Fspace.c
    ${HDF5_SRC_DIR}/H5Fsuper.c
    ${HDF5_SRC_DIR}/H5Fsuper_cache.c
    ${HDF5_SRC_DIR}/H5Ftest.c
)

set (H5F_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Fpublic.h
)
IDE_GENERATED_PROPERTIES ("H5F" "${H5F_HDRS}" "${H5F_SOURCES}" )

set (H5F_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Fpkg.h
    ${HDF5_SRC_DIR}/H5Fprivate.h
)


set (H5FA_SOURCES
    ${HDF5_SRC_DIR}/H5FA.c
    ${HDF5_SRC_DIR}/H5FAcache.c
    ${HDF5_SRC_DIR}/H5FAdbg.c
    ${HDF5_SRC_DIR}/H5FAdblkpage.c
    ${HDF5_SRC_DIR}/H5FAdblock.c
    ${HDF5_SRC_DIR}/H5FAhdr.c
    ${HDF5_SRC_DIR}/H5FAint.c
    ${HDF5_SRC_DIR}/H5FAstat.c
    ${HDF5_SRC_DIR}/H5FAtest.c
)
set (H5FA_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FA" "${H5FA_HDRS}" "${H5FA_SOURCES}" )

set (H5FA_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5FApkg.h
    ${HDF5_SRC_DIR}/H5FAprivate.h
)


set (H5FD_SOURCES
    ${HDF5_SRC_DIR}/H5FD.c
    ${HDF5_SRC_DIR}/H5FDcore.c
    ${HDF5_SRC_DIR}/H5FDdirect.c
    ${HDF5_SRC_DIR}/H5FDfamily.c
    ${HDF5_SRC_DIR}/H5FDhdfs.c
    ${HDF5_SRC_DIR}/H5FDint.c
    ${HDF5_SRC_DIR}/H5FDlog.c
    ${HDF5_SRC_DIR}/H5FDmirror.c
    ${HDF5_SRC_DIR}/H5FDmpi.c
    ${HDF5_SRC_DIR}/H5FDmpio.c
    ${HDF5_SRC_DIR}/H5FDmulti.c
    ${HDF5_SRC_DIR}/H5FDmulti_int.c
    ${HDF5_SRC_DIR}/H5FDonion.c
    ${HDF5_SRC_DIR}/H5FDonion_header.c
    ${HDF5_SRC_DIR}/H5FDonion_history.c
    ${HDF5_SRC_DIR}/H5FDonion_index.c
    ${HDF5_SRC_DIR}/H5FDros3.c
    ${HDF5_SRC_DIR}/H5FDros3_s3comms.c
    ${HDF5_SRC_DIR}/H5FDsec2.c
    ${HDF5_SRC_DIR}/H5FDspace.c
    ${HDF5_SRC_DIR}/H5FDsplitter.c
    ${HDF5_SRC_DIR}/H5FDstdio.c
    ${HDF5_SRC_DIR}/H5FDstdio_int.c
    ${HDF5_SRC_DIR}/H5FDtest.c
    ${HDF5_SRC_DIR}/H5FDwindows.c
)

set (H5FD_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5FDcore.h
    ${HDF5_SRC_DIR}/H5FDdevelop.h
    ${HDF5_SRC_DIR}/H5FDdirect.h
    ${HDF5_SRC_DIR}/H5FDfamily.h
    ${HDF5_SRC_DIR}/H5FDhdfs.h
    ${HDF5_SRC_DIR}/H5FDlog.h
    ${HDF5_SRC_DIR}/H5FDmirror.h
    ${HDF5_SRC_DIR}/H5FDmpi.h
    ${HDF5_SRC_DIR}/H5FDmpio.h
    ${HDF5_SRC_DIR}/H5FDmulti.h
    ${HDF5_SRC_DIR}/H5FDonion.h
    ${HDF5_SRC_DIR}/H5FDpublic.h
    ${HDF5_SRC_DIR}/H5FDros3.h
    ${HDF5_SRC_DIR}/H5FDsec2.h
    ${HDF5_SRC_DIR}/H5FDsplitter.h
    ${HDF5_SRC_DIR}/H5FDstdio.h
    ${HDF5_SRC_DIR}/H5FDwindows.h
)

# Append Subfiling VFD sources to H5FD interface if it is built.
# Append Subfiling VFD public headers to H5FD_HDRS regardless.
add_subdirectory (${H5FD_SUBFILING_DIR})

IDE_GENERATED_PROPERTIES ("H5FD" "${H5FD_HDRS}" "${H5FD_SOURCES}" )

set (H5FD_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5FDmirror_priv.h
    ${HDF5_SRC_DIR}/H5FDmulti_private.h
    ${HDF5_SRC_DIR}/H5FDonion_header.h
    ${HDF5_SRC_DIR}/H5FDonion_history.h
    ${HDF5_SRC_DIR}/H5FDonion_index.h
    ${HDF5_SRC_DIR}/H5FDonion_priv.h
    ${HDF5_SRC_DIR}/H5FDpkg.h
    ${HDF5_SRC_DIR}/H5FDprivate.h
    ${HDF5_SRC_DIR}/H5FDros3_s3comms.h
    ${HDF5_SRC_DIR}/H5FDstdio_private.h
)


set (H5FL_SOURCES
    ${HDF5_SRC_DIR}/H5FL.c
)
set (H5FL_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FL" "${H5FL_HDRS}" "${H5FL_SOURCES}" )

set (H5FL_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5FLprivate.h
)


set (H5FO_SOURCES
    ${HDF5_SRC_DIR}/H5FO.c
)
set (H5FO_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FO" "${H5FO_HDRS}" "${H5FO_SOURCES}" )

set (H5FO_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5FOprivate.h
)


set (H5FS_SOURCES
    ${HDF5_SRC_DIR}/H5FS.c
    ${HDF5_SRC_DIR}/H5FScache.c
    ${HDF5_SRC_DIR}/H5FSdbg.c
    ${HDF5_SRC_DIR}/H5FSint.c
    ${HDF5_SRC_DIR}/H5FSsection.c
    ${HDF5_SRC_DIR}/H5FSstat.c
    ${HDF5_SRC_DIR}/H5FStest.c
)

set (H5FS_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FS" "${H5FS_HDRS}" "${H5FS_SOURCES}" )

set (H5FS_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5FSpkg.h
    ${HDF5_SRC_DIR}/H5FSprivate.h
)


set (H5G_SOURCES
    ${HDF5_SRC_DIR}/H5G.c
    ${HDF5_SRC_DIR}/H5Gbtree2.c
    ${HDF5_SRC_DIR}/H5Gcache.c
    ${HDF5_SRC_DIR}/H5Gcompact.c
    ${HDF5_SRC_DIR}/H5Gdense.c
    ${HDF5_SRC_DIR}/H5Gdeprec.c
    ${HDF5_SRC_DIR}/H5Gent.c
    ${HDF5_SRC_DIR}/H5Gint.c
    ${HDF5_SRC_DIR}/H5Glink.c
    ${HDF5_SRC_DIR}/H5Gloc.c
    ${HDF5_SRC_DIR}/H5Gname.c
    ${HDF5_SRC_DIR}/H5Gnode.c
    ${HDF5_SRC_DIR}/H5Gobj.c
    ${HDF5_SRC_DIR}/H5Goh.c
    ${HDF5_SRC_DIR}/H5Groot.c
    ${HDF5_SRC_DIR}/H5Gstab.c
    ${HDF5_SRC_DIR}/H5Gtest.c
    ${HDF5_SRC_DIR}/H5Gtraverse.c
)

set (H5G_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Gpublic.h
)
IDE_GENERATED_PROPERTIES ("H5G" "${H5G_HDRS}" "${H5G_SOURCES}" )

set (H5G_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Gpkg.h
    ${HDF5_SRC_DIR}/H5Gprivate.h
)


set (H5HF_SOURCES
    ${HDF5_SRC_DIR}/H5HF.c
    ${HDF5_SRC_DIR}/H5HFbtree2.c
    ${HDF5_SRC_DIR}/H5HFcache.c
    ${HDF5_SRC_DIR}/H5HFdbg.c
    ${HDF5_SRC_DIR}/H5HFdblock.c
    ${HDF5_SRC_DIR}/H5HFdtable.c
    ${HDF5_SRC_DIR}/H5HFhdr.c
    ${HDF5_SRC_DIR}/H5HFhuge.c
    ${HDF5_SRC_DIR}/H5HFiblock.c
    ${HDF5_SRC_DIR}/H5HFiter.c
    ${HDF5_SRC_DIR}/H5HFman.c
    ${HDF5_SRC_DIR}/H5HFsection.c
    ${HDF5_SRC_DIR}/H5HFspace.c
    ${HDF5_SRC_DIR}/H5HFstat.c
    ${HDF5_SRC_DIR}/H5HFtest.c
    ${HDF5_SRC_DIR}/H5HFtiny.c
)

set (H5HF_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5HF" "${H5HF_HDRS}" "${H5HF_SOURCES}" )

set (H5HF_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5HFpkg.h
    ${HDF5_SRC_DIR}/H5HFprivate.h
)


set (H5HG_SOURCES
    ${HDF5_SRC_DIR}/H5HG.c
    ${HDF5_SRC_DIR}/H5HGcache.c
    ${HDF5_SRC_DIR}/H5HGdbg.c
    ${HDF5_SRC_DIR}/H5HGquery.c
)

set (H5HG_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5HG" "${H5HG_HDRS}" "${H5HG_SOURCES}" )

set (H5HG_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5HGpkg.h
    ${HDF5_SRC_DIR}/H5HGprivate.h
)


set (H5HL_SOURCES
    ${HDF5_SRC_DIR}/H5HL.c
    ${HDF5_SRC_DIR}/H5HLcache.c
    ${HDF5_SRC_DIR}/H5HLdbg.c
    ${HDF5_SRC_DIR}/H5HLdblk.c
    ${HDF5_SRC_DIR}/H5HLint.c
    ${HDF5_SRC_DIR}/H5HLprfx.c
)

set (H5HL_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5HL" "${H5HL_HDRS}" "${H5HL_SOURCES}" )

set (H5HL_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5HLpkg.h
    ${HDF5_SRC_DIR}/H5HLprivate.h
)


set (H5I_SOURCES
    ${HDF5_SRC_DIR}/H5I.c
    ${HDF5_SRC_DIR}/H5Idbg.c
    ${HDF5_SRC_DIR}/H5Ideprec.c
    ${HDF5_SRC_DIR}/H5Iint.c
    ${HDF5_SRC_DIR}/H5Itest.c
)
set (H5I_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Idevelop.h
    ${HDF5_SRC_DIR}/H5Ipublic.h
)
IDE_GENERATED_PROPERTIES ("H5I" "${H5I_HDRS}" "${H5I_SOURCES}" )

set (H5I_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Ipkg.h
    ${HDF5_SRC_DIR}/H5Iprivate.h
)


set (H5L_SOURCES
    ${HDF5_SRC_DIR}/H5L.c
    ${HDF5_SRC_DIR}/H5Ldeprec.c
    ${HDF5_SRC_DIR}/H5Lexternal.c
    ${HDF5_SRC_DIR}/H5Lint.c
)
set (H5L_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Ldevelop.h
    ${HDF5_SRC_DIR}/H5Lpublic.h
)
IDE_GENERATED_PROPERTIES ("H5L" "${H5L_HDRS}" "${H5L_SOURCES}" )

set (H5L_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Lpkg.h
    ${HDF5_SRC_DIR}/H5Lprivate.h
)


set (H5M_SOURCES
    ${HDF5_SRC_DIR}/H5M.c
)
set (H5M_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Mpublic.h
)
IDE_GENERATED_PROPERTIES ("H5M" "${H5M_HDRS}" "${H5M_SOURCES}")

set (H5M_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Mpkg.h
    ${HDF5_SRC_DIR}/H5Mprivate.h
)


set (H5MF_SOURCES
    ${HDF5_SRC_DIR}/H5MF.c
    ${HDF5_SRC_DIR}/H5MFaggr.c
    ${HDF5_SRC_DIR}/H5MFdbg.c
    ${HDF5_SRC_DIR}/H5MFsection.c
)

set (H5MF_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5MF" "${H5MF_HDRS}" "${H5MF_SOURCES}" )

set (H5MF_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5MFpkg.h
    ${HDF5_SRC_DIR}/H5MFprivate.h
)


set (H5MM_SOURCES
    ${HDF5_SRC_DIR}/H5MM.c
)
set (H5MM_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5MMpublic.h
)
IDE_GENERATED_PROPERTIES ("H5MM" "${H5MM_HDRS}" "${H5MM_SOURCES}" )

set (H5MM_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5MMprivate.h
)


set (H5O_SOURCES
    ${HDF5_SRC_DIR}/H5O.c
    ${HDF5_SRC_DIR}/H5Oainfo.c
    ${HDF5_SRC_DIR}/H5Oalloc.c
    ${HDF5_SRC_DIR}/H5Oattr.c
    ${HDF5_SRC_DIR}/H5Oattribute.c
    ${HDF5_SRC_DIR}/H5Obogus.c
    ${HDF5_SRC_DIR}/H5Obtreek.c
    ${HDF5_SRC_DIR}/H5Ocache.c
    ${HDF5_SRC_DIR}/H5Ocache_image.c
    ${HDF5_SRC_DIR}/H5Ochunk.c
    ${HDF5_SRC_DIR}/H5Ocont.c
    ${HDF5_SRC_DIR}/H5Ocopy.c
    ${HDF5_SRC_DIR}/H5Ocopy_ref.c
    ${HDF5_SRC_DIR}/H5Odbg.c
    ${HDF5_SRC_DIR}/H5Odeprec.c
    ${HDF5_SRC_DIR}/H5Odrvinfo.c
    ${HDF5_SRC_DIR}/H5Odtype.c
    ${HDF5_SRC_DIR}/H5Oefl.c
    ${HDF5_SRC_DIR}/H5Ofill.c
    ${HDF5_SRC_DIR}/H5Oflush.c
    ${HDF5_SRC_DIR}/H5Ofsinfo.c
    ${HDF5_SRC_DIR}/H5Oginfo.c
    ${HDF5_SRC_DIR}/H5Oint.c
    ${HDF5_SRC_DIR}/H5Olayout.c
    ${HDF5_SRC_DIR}/H5Olinfo.c
    ${HDF5_SRC_DIR}/H5Olink.c
    ${HDF5_SRC_DIR}/H5Omessage.c
    ${HDF5_SRC_DIR}/H5Omtime.c
    ${HDF5_SRC_DIR}/H5Oname.c
    ${HDF5_SRC_DIR}/H5Onull.c
    ${HDF5_SRC_DIR}/H5Opline.c
    ${HDF5_SRC_DIR}/H5Orefcount.c
    ${HDF5_SRC_DIR}/H5Osdspace.c
    ${HDF5_SRC_DIR}/H5Oshared.c
    ${HDF5_SRC_DIR}/H5Oshmesg.c
    ${HDF5_SRC_DIR}/H5Ostab.c
    ${HDF5_SRC_DIR}/H5Otest.c
    ${HDF5_SRC_DIR}/H5Ounknown.c
)

set (H5O_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Opublic.h
)
IDE_GENERATED_PROPERTIES ("H5O" "${H5O_HDRS}" "${H5O_SOURCES}" )

set (H5O_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Opkg.h
    ${HDF5_SRC_DIR}/H5Oprivate.h
    ${HDF5_SRC_DIR}/H5Oshared.h
)


set (H5P_SOURCES
    ${HDF5_SRC_DIR}/H5P.c
    ${HDF5_SRC_DIR}/H5Pacpl.c
    ${HDF5_SRC_DIR}/H5Pdapl.c
    ${HDF5_SRC_DIR}/H5Pdcpl.c
    ${HDF5_SRC_DIR}/H5Pdeprec.c
    ${HDF5_SRC_DIR}/H5Pdxpl.c
    ${HDF5_SRC_DIR}/H5Pencdec.c
    ${HDF5_SRC_DIR}/H5Pfapl.c
    ${HDF5_SRC_DIR}/H5Pfcpl.c
    ${HDF5_SRC_DIR}/H5Pfmpl.c
    ${HDF5_SRC_DIR}/H5Pgcpl.c
    ${HDF5_SRC_DIR}/H5Pint.c
    ${HDF5_SRC_DIR}/H5Plapl.c
    ${HDF5_SRC_DIR}/H5Plcpl.c
    ${HDF5_SRC_DIR}/H5Pmapl.c
    ${HDF5_SRC_DIR}/H5Pmcpl.c
    ${HDF5_SRC_DIR}/H5Pocpl.c
    ${HDF5_SRC_DIR}/H5Pocpypl.c
    ${HDF5_SRC_DIR}/H5Pstrcpl.c
    ${HDF5_SRC_DIR}/H5Ptest.c
)

set (H5P_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Ppublic.h
)
IDE_GENERATED_PROPERTIES ("H5P" "${H5P_HDRS}" "${H5P_SOURCES}" )

set (H5P_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Ppkg.h
    ${HDF5_SRC_DIR}/H5Pprivate.h
)


set (H5PB_SOURCES
    ${HDF5_SRC_DIR}/H5PB.c
)

set (H5PB_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5PB" "${H5PB_HDRS}" "${H5PB_SOURCES}" )

set (H5PB_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5PBprivate.h
    ${HDF5_SRC_DIR}/H5PBpkg.h
)


set (H5PL_SOURCES
    ${HDF5_SRC_DIR}/H5PL.c
    ${HDF5_SRC_DIR}/H5PLint.c
    ${HDF5_SRC_DIR}/H5PLpath.c
    ${HDF5_SRC_DIR}/H5PLplugin_cache.c
)

set (H5PL_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5PLextern.h
    ${HDF5_SRC_DIR}/H5PLpublic.h
)
IDE_GENERATED_PROPERTIES ("H5PL" "${H5PL_HDRS}" "${H5PL_SOURCES}" )

set (H5PL_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5PLpkg.h
    ${HDF5_SRC_DIR}/H5PLprivate.h
)


set (H5R_SOURCES
    ${HDF5_SRC_DIR}/H5R.c
    ${HDF5_SRC_DIR}/H5Rdeprec.c
    ${HDF5_SRC_DIR}/H5Rint.c
)
set (H5R_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Rpublic.h
)
IDE_GENERATED_PROPERTIES ("H5R" "${H5R_HDRS}" "${H5R_SOURCES}" )

set (H5R_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Rpkg.h
    ${HDF5_SRC_DIR}/H5Rprivate.h
)


set (H5RS_SOURCES
    ${HDF5_SRC_DIR}/H5RS.c
)
set (H5RS_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5RS" "${H5RS_HDRS}" "${H5RS_SOURCES}" )

set (H5RS_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5RSprivate.h
)


set (H5S_SOURCES
    ${HDF5_SRC_DIR}/H5S.c
    ${HDF5_SRC_DIR}/H5Sall.c
    ${HDF5_SRC_DIR}/H5Sdbg.c
    ${HDF5_SRC_DIR}/H5Sdeprec.c
    ${HDF5_SRC_DIR}/H5Shyper.c
    ${HDF5_SRC_DIR}/H5Smpio.c
    ${HDF5_SRC_DIR}/H5Snone.c
    ${HDF5_SRC_DIR}/H5Spoint.c
    ${HDF5_SRC_DIR}/H5Sselect.c
    ${HDF5_SRC_DIR}/H5Stest.c
)

set (H5S_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Spublic.h
)
IDE_GENERATED_PROPERTIES ("H5S" "${H5S_HDRS}" "${H5S_SOURCES}" )

set (H5S_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Spkg.h
    ${HDF5_SRC_DIR}/H5Sprivate.h
)


set (H5SL_SOURCES
    ${HDF5_SRC_DIR}/H5SL.c
)
set (H5SL_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5SL" "${H5SL_HDRS}" "${H5SL_SOURCES}" )

set (H5SL_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5SLprivate.h
)


set (H5SM_SOURCES
    ${HDF5_SRC_DIR}/H5SM.c
    ${HDF5_SRC_DIR}/H5SMbtree2.c
    ${HDF5_SRC_DIR}/H5SMcache.c
    ${HDF5_SRC_DIR}/H5SMmessage.c
    ${HDF5_SRC_DIR}/H5SMtest.c
)

set (H5SM_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5SM" "${H5SM_HDRS}" "${H5SM_SOURCES}" )

set (H5SM_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5SMpkg.h
    ${HDF5_SRC_DIR}/H5SMprivate.h
)


set (H5T_SOURCES
    ${HDF5_SRC_DIR}/H5T.c
    ${HDF5_SRC_DIR}/H5Tarray.c
    ${HDF5_SRC_DIR}/H5Tbit.c
    ${HDF5_SRC_DIR}/H5Tcommit.c
    ${HDF5_SRC_DIR}/H5Tcomplex.c
    ${HDF5_SRC_DIR}/H5Tcompound.c
    ${HDF5_SRC_DIR}/H5Tconv.c
    ${HDF5_SRC_DIR}/H5Tconv_array.c
    ${HDF5_SRC_DIR}/H5Tconv_bitfield.c
    ${HDF5_SRC_DIR}/H5Tconv_complex.c
    ${HDF5_SRC_DIR}/H5Tconv_compound.c
    ${HDF5_SRC_DIR}/H5Tconv_enum.c
    ${HDF5_SRC_DIR}/H5Tconv_float.c
    ${HDF5_SRC_DIR}/H5Tconv_integer.c
    ${HDF5_SRC_DIR}/H5Tconv_reference.c
    ${HDF5_SRC_DIR}/H5Tconv_string.c
    ${HDF5_SRC_DIR}/H5Tconv_vlen.c
    ${HDF5_SRC_DIR}/H5Tcset.c
    ${HDF5_SRC_DIR}/H5Tdbg.c
    ${HDF5_SRC_DIR}/H5Tdeprec.c
    ${HDF5_SRC_DIR}/H5Tenum.c
    ${HDF5_SRC_DIR}/H5Tfields.c
    ${HDF5_SRC_DIR}/H5Tfixed.c
    ${HDF5_SRC_DIR}/H5Tfloat.c
    ${HDF5_SRC_DIR}/H5Tinit_float.c
    ${HDF5_SRC_DIR}/H5Tnative.c
    ${HDF5_SRC_DIR}/H5Toffset.c
    ${HDF5_SRC_DIR}/H5Toh.c
    ${HDF5_SRC_DIR}/H5Topaque.c
    ${HDF5_SRC_DIR}/H5Torder.c
    ${HDF5_SRC_DIR}/H5Tpad.c
    ${HDF5_SRC_DIR}/H5Tprecis.c
    ${HDF5_SRC_DIR}/H5Tref.c
    ${HDF5_SRC_DIR}/H5Tstrpad.c
    ${HDF5_SRC_DIR}/H5Tvisit.c
    ${HDF5_SRC_DIR}/H5Tvlen.c
)

set (H5T_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Tdevelop.h
    ${HDF5_SRC_DIR}/H5Tpublic.h
)
IDE_GENERATED_PROPERTIES ("H5T" "${H5T_HDRS}" "${H5T_SOURCES}" )

set (H5T_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Tconv.h
    ${HDF5_SRC_DIR}/H5Tconv_array.h
    ${HDF5_SRC_DIR}/H5Tconv_bitfield.h
    ${HDF5_SRC_DIR}/H5Tconv_complex.h
    ${HDF5_SRC_DIR}/H5Tconv_compound.h
    ${HDF5_SRC_DIR}/H5Tconv_enum.h
    ${HDF5_SRC_DIR}/H5Tconv_float.h
    ${HDF5_SRC_DIR}/H5Tconv_integer.h
    ${HDF5_SRC_DIR}/H5Tconv_macros.h
    ${HDF5_SRC_DIR}/H5Tconv_reference.h
    ${HDF5_SRC_DIR}/H5Tconv_string.h
    ${HDF5_SRC_DIR}/H5Tconv_vlen.h
    ${HDF5_SRC_DIR}/H5Tpkg.h
    ${HDF5_SRC_DIR}/H5Tprivate.h
)


set (H5TS_SOURCES
    ${HDF5_SRC_DIR}/H5TS.c
    ${HDF5_SRC_DIR}/H5TSatomic.c
    ${HDF5_SRC_DIR}/H5TSbarrier.c
    ${HDF5_SRC_DIR}/H5TSc11.c
    ${HDF5_SRC_DIR}/H5TScond.c
    ${HDF5_SRC_DIR}/H5TSint.c
    ${HDF5_SRC_DIR}/H5TSkey.c
    ${HDF5_SRC_DIR}/H5TSmutex.c
    ${HDF5_SRC_DIR}/H5TSonce.c
    ${HDF5_SRC_DIR}/H5TSpool.c
    ${HDF5_SRC_DIR}/H5TSpthread.c
    ${HDF5_SRC_DIR}/H5TSrec_rwlock.c
    ${HDF5_SRC_DIR}/H5TSrwlock.c
    ${HDF5_SRC_DIR}/H5TSsemaphore.c
    ${HDF5_SRC_DIR}/H5TSthread.c
    ${HDF5_SRC_DIR}/H5TSwin.c
)
set (H5TS_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5TSdevelop.h
)
IDE_GENERATED_PROPERTIES ("H5TS" "${H5TS_HDRS}" "${H5TS_SOURCES}" )

set (H5TS_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5TSpkg.h
    ${HDF5_SRC_DIR}/H5TSprivate.h
)


set (H5UC_SOURCES
    ${HDF5_SRC_DIR}/H5UC.c
)
set (H5UC_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5UC" "${H5UC_HDRS}" "${H5UC_SOURCES}" )

set (H5UC_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5UCprivate.h
)


set (H5VL_SOURCES
    ${HDF5_SRC_DIR}/H5VL.c
    ${HDF5_SRC_DIR}/H5VLcallback.c
    ${HDF5_SRC_DIR}/H5VLdyn_ops.c
    ${HDF5_SRC_DIR}/H5VLint.c
    ${HDF5_SRC_DIR}/H5VLnative.c
    ${HDF5_SRC_DIR}/H5VLnative_attr.c
    ${HDF5_SRC_DIR}/H5VLnative_blob.c
    ${HDF5_SRC_DIR}/H5VLnative_dataset.c
    ${HDF5_SRC_DIR}/H5VLnative_datatype.c
    ${HDF5_SRC_DIR}/H5VLnative_file.c
    ${HDF5_SRC_DIR}/H5VLnative_group.c
    ${HDF5_SRC_DIR}/H5VLnative_link.c
    ${HDF5_SRC_DIR}/H5VLnative_introspect.c
    ${HDF5_SRC_DIR}/H5VLnative_object.c
    ${HDF5_SRC_DIR}/H5VLnative_token.c
    ${HDF5_SRC_DIR}/H5VLpassthru.c
    ${HDF5_SRC_DIR}/H5VLpassthru_int.c
    ${HDF5_SRC_DIR}/H5VLquery.c
    ${HDF5_SRC_DIR}/H5VLtest.c
)
set (H5VL_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5VLconnector.h
    ${HDF5_SRC_DIR}/H5VLconnector_passthru.h
    ${HDF5_SRC_DIR}/H5VLnative.h
    ${HDF5_SRC_DIR}/H5VLpassthru.h
    ${HDF5_SRC_DIR}/H5VLpublic.h
)
IDE_GENERATED_PROPERTIES ("H5VL" "${H5VL_HDRS}" "${H5VL_SOURCES}" )

set (H5VL_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5VLnative_private.h
    ${HDF5_SRC_DIR}/H5VLpkg.h
    ${HDF5_SRC_DIR}/H5VLprivate.h
)


set (H5VM_SOURCES
    ${HDF5_SRC_DIR}/H5VM.c
)
set (H5VM_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5VM" "${H5VM_HDRS}" "${H5VM_SOURCES}" )

set (H5VM_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5VMprivate.h
)


set (H5WB_SOURCES
    ${HDF5_SRC_DIR}/H5WB.c
)
set (H5WB_PUBLIC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5WB" "${H5WB_HDRS}" "${H5WB_SOURCES}" )

set (H5WB_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5WBprivate.h
)


set (H5Z_SOURCES
    ${HDF5_SRC_DIR}/H5Z.c
    ${HDF5_SRC_DIR}/H5Zdeflate.c
    ${HDF5_SRC_DIR}/H5Zfletcher32.c
    ${HDF5_SRC_DIR}/H5Znbit.c
    ${HDF5_SRC_DIR}/H5Zscaleoffset.c
    ${HDF5_SRC_DIR}/H5Zshuffle.c
    ${HDF5_SRC_DIR}/H5Zszip.c
    ${HDF5_SRC_DIR}/H5Ztrans.c
)
if (H5_ZLIB_HEADER)
  message(VERBOSE "H5_ZLIB_HEADER for library=${H5_ZLIB_HEADER}")
  set_source_files_properties(${HDF5_SRC_DIR}/H5Zdeflate.c
               PROPERTIES COMPILE_DEFINITIONS H5_ZLIB_HEADER="${H5_ZLIB_HEADER}"
  )
endif ()

set (H5Z_PUBLIC_HDRS
    ${HDF5_SRC_DIR}/H5Zdevelop.h
    ${HDF5_SRC_DIR}/H5Zpublic.h
)
IDE_GENERATED_PROPERTIES ("H5Z" "${H5Z_HDRS}" "${H5Z_SOURCES}" )

set (H5Z_PRIVATE_HDRS
    ${HDF5_SRC_DIR}/H5Zpkg.h
    ${HDF5_SRC_DIR}/H5Zprivate.h
)


set (H5_MODULE_HEADERS
    ${HDF5_SRC_DIR}/H5module.h
    ${HDF5_SRC_DIR}/H5Amodule.h
    ${HDF5_SRC_DIR}/H5ACmodule.h
    ${HDF5_SRC_DIR}/H5Bmodule.h
    ${HDF5_SRC_DIR}/H5B2module.h
    ${HDF5_SRC_DIR}/H5Cmodule.h
    ${HDF5_SRC_DIR}/H5CXmodule.h
    ${HDF5_SRC_DIR}/H5Dmodule.h
    ${HDF5_SRC_DIR}/H5Emodule.h
    ${HDF5_SRC_DIR}/H5EAmodule.h
    ${HDF5_SRC_DIR}/H5ESmodule.h
    ${HDF5_SRC_DIR}/H5Fmodule.h
    ${HDF5_SRC_DIR}/H5FAmodule.h
    ${HDF5_SRC_DIR}/H5FDmodule.h
    ${HDF5_SRC_DIR}/H5FLmodule.h
    ${HDF5_SRC_DIR}/H5FSmodule.h
    ${HDF5_SRC_DIR}/H5Gmodule.h
    ${HDF5_SRC_DIR}/H5HFmodule.h
    ${HDF5_SRC_DIR}/H5HGmodule.h
    ${HDF5_SRC_DIR}/H5HLmodule.h
    ${HDF5_SRC_DIR}/H5Imodule.h
    ${HDF5_SRC_DIR}/H5Lmodule.h
    ${HDF5_SRC_DIR}/H5Mmodule.h
    ${HDF5_SRC_DIR}/H5MFmodule.h
    ${HDF5_SRC_DIR}/H5Omodule.h
    ${HDF5_SRC_DIR}/H5Pmodule.h
    ${HDF5_SRC_DIR}/H5PBmodule.h
    ${HDF5_SRC_DIR}/H5PLmodule.h
    ${HDF5_SRC_DIR}/H5Rmodule.h
    ${HDF5_SRC_DIR}/H5RSmodule.h
    ${HDF5_SRC_DIR}/H5Smodule.h
    ${HDF5_SRC_DIR}/H5SLmodule.h
    ${HDF5_SRC_DIR}/H5SMmodule.h
    ${HDF5_SRC_DIR}/H5Tmodule.h
    ${HDF5_SRC_DIR}/H5TSmodule.h
    ${HDF5_SRC_DIR}/H5VLmodule.h
    ${HDF5_SRC_DIR}/H5Zmodule.h
)

set (common_SRCS
    ${H5_SOURCES}
    ${H5A_SOURCES}
    ${H5AC_SOURCES}
    ${H5B_SOURCES}
    ${H5B2_SOURCES}
    ${H5C_SOURCES}
    ${H5CX_SOURCES}
    ${H5D_SOURCES}
    ${H5E_SOURCES}
    ${H5EA_SOURCES}
    ${H5ES_SOURCES}
    ${H5F_SOURCES}
    ${H5FA_SOURCES}
    ${H5FD_SOURCES}
    ${H5FL_SOURCES}
    ${H5FO_SOURCES}
    ${H5FS_SOURCES}
    ${H5G_SOURCES}
    ${H5HF_SOURCES}
    ${H5HG_SOURCES}
    ${H5HL_SOURCES}
    ${H5I_SOURCES}
    ${H5L_SOURCES}
    ${H5M_SOURCES}
    ${H5MF_SOURCES}
    ${H5MM_SOURCES}
    ${H5O_SOURCES}
    ${H5P_SOURCES}
    ${H5PB_SOURCES}
    ${H5PL_SOURCES}
    ${H5R_SOURCES}
    ${H5UC_SOURCES}
    ${H5RS_SOURCES}
    ${H5S_SOURCES}
    ${H5SL_SOURCES}
    ${H5SM_SOURCES}
    ${H5T_SOURCES}
    ${H5TS_SOURCES}
    ${H5VL_SOURCES}
    ${H5VM_SOURCES}
    ${H5WB_SOURCES}
    ${H5Z_SOURCES}
)

set (H5_PUBLIC_HEADERS
    ${H5_PUBLIC_HDRS}
    ${H5A_PUBLIC_HDRS}
    ${H5AC_PUBLIC_HDRS}
    ${H5B_PUBLIC_HDRS}
    ${H5B2_PUBLIC_HDRS}
    ${H5C_PUBLIC_HDRS}
    ${H5D_PUBLIC_HDRS}
    ${H5E_PUBLIC_HDRS}
    ${H5EA_PUBLIC_HDRS}
    ${H5ES_PUBLIC_HDRS}
    ${H5F_PUBLIC_HDRS}
    ${H5FA_PUBLIC_HDRS}
    ${H5FD_PUBLIC_HDRS}
    ${H5FS_PUBLIC_HDRS}
    ${H5G_PUBLIC_HDRS}
    ${H5HF_PUBLIC_HDRS}
    ${H5HG_PUBLIC_HDRS}
    ${H5HL_PUBLIC_HDRS}
    ${H5I_PUBLIC_HDRS}
    ${H5L_PUBLIC_HDRS}
    ${H5M_PUBLIC_HDRS}
    ${H5MF_PUBLIC_HDRS}
    ${H5MM_PUBLIC_HDRS}
    ${H5O_PUBLIC_HDRS}
    ${H5P_PUBLIC_HDRS}
    ${H5PB_PUBLIC_HDRS}
    ${H5PL_PUBLIC_HDRS}
    ${H5R_PUBLIC_HDRS}
    ${H5S_PUBLIC_HDRS}
    ${H5SM_PUBLIC_HDRS}
    ${H5T_PUBLIC_HDRS}
    ${H5TS_PUBLIC_HDRS}
    ${H5VL_PUBLIC_HDRS}
    ${H5Z_PUBLIC_HDRS}
)

set (H5_PRIVATE_HEADERS
    ${H5_PRIVATE_HDRS}
    ${H5A_PRIVATE_HDRS}
    ${H5AC_PRIVATE_HDRS}
    ${H5B_PRIVATE_HDRS}
    ${H5B2_PRIVATE_HDRS}
    ${H5C_PRIVATE_HDRS}
    ${H5D_PRIVATE_HDRS}
    ${H5E_PRIVATE_HDRS}
    ${H5EA_PRIVATE_HDRS}
    ${H5ES_PRIVATE_HDRS}
    ${H5F_PRIVATE_HDRS}
    ${H5FA_PRIVATE_HDRS}
    ${H5FD_PRIVATE_HDRS}
    ${H5FS_PRIVATE_HDRS}
    ${H5G_PRIVATE_HDRS}
    ${H5HF_PRIVATE_HDRS}
    ${H5HG_PRIVATE_HDRS}
    ${H5HL_PRIVATE_HDRS}
    ${H5I_PRIVATE_HDRS}
    ${H5L_PRIVATE_HDRS}
    ${H5M_PRIVATE_HDRS}
    ${H5MF_PRIVATE_HDRS}
    ${H5MM_PRIVATE_HDRS}
    ${H5O_PRIVATE_HDRS}
    ${H5P_PRIVATE_HDRS}
    ${H5PB_PRIVATE_HDRS}
    ${H5PL_PRIVATE_HDRS}
    ${H5R_PRIVATE_HDRS}
    ${H5S_PRIVATE_HDRS}
    ${H5SM_PRIVATE_HDRS}
    ${H5T_PRIVATE_HDRS}
    ${H5TS_PRIVATE_HDRS}
    ${H5VL_PRIVATE_HDRS}
    ${H5Z_PRIVATE_HDRS}
)

set (H5_GENERATED_HEADERS
    ${HDF5_SRC_DIR}/H5Edefin.h
    ${HDF5_SRC_DIR}/H5Einit.h
    ${HDF5_SRC_DIR}/H5Epubgen.h
    ${HDF5_SRC_DIR}/H5Eterm.h
    ${HDF5_SRC_DIR}/H5version.h
    ${HDF5_SRC_DIR}/H5overflow.h
)

set (H5_PUBLIC_GENERATED_HEADERS
    ${HDF5_SRC_DIR}/H5Epubgen.h
    ${HDF5_SRC_DIR}/H5version.h
    ${HDF5_SRC_DIR}/H5overflow.h
)

option (HDF5_GENERATE_HEADERS "Rebuild Generated Files" OFF)
if (HDF5_GENERATE_HEADERS)
  set_source_files_properties(${H5_GENERATED_HEADERS} PROPERTIES GENERATED TRUE)
  if (H5_PERL_FOUND)
    execute_process (
        COMMAND ${PERL_EXECUTABLE} ${HDF5_SOURCE_DIR}/bin/make_err ${HDF5_SRC_DIR}/H5err.txt OUTPUT_VARIABLE SCRIPT_OUTPUT
    )
    message(STATUS ${SCRIPT_OUTPUT})
    execute_process (
        COMMAND ${PERL_EXECUTABLE} ${HDF5_SOURCE_DIR}/bin/make_vers ${HDF5_SRC_DIR}/H5vers.txt OUTPUT_VARIABLE SCRIPT_OUTPUT
    )
    message(STATUS ${SCRIPT_OUTPUT})
    execute_process (
        COMMAND ${PERL_EXECUTABLE} ${HDF5_SOURCE_DIR}/bin/make_overflow ${HDF5_SRC_DIR}/H5overflow.txt OUTPUT_VARIABLE SCRIPT_OUTPUT
    )
    message(STATUS ${SCRIPT_OUTPUT})
  else ()
    message (WARNING "Cannot generate headers - perl not found")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Generate the H5build_settings.c file
#-----------------------------------------------------------------------------
configure_file (${HDF5_SOURCE_DIR}/src/H5build_settings.cmake.c.in ${HDF5_SRC_BINARY_DIR}/H5build_settings.c @ONLY)

if (BUILD_SHARED_LIBS)
  file (MAKE_DIRECTORY "${HDF5_SRC_BINARY_DIR}/shared")
endif ()

if (LOCAL_BATCH_TEST)
    configure_file (
      ${HDF5_SOURCE_DIR}/bin/batch/ctest_serial.cmake.in
      ${HDF5_BINARY_DIR}/ctest_serial.cmake ESCAPE_QUOTES @ONLY
    )
    configure_file (
      ${HDF5_SOURCE_DIR}/bin/batch/ctest_parallel.cmake.in
      ${HDF5_BINARY_DIR}/ctest_parallel.cmake ESCAPE_QUOTES @ONLY
    )
  if (LOCAL_BATCH_SCRIPT_COMMAND STREQUAL "raybsub")
    configure_file (
        ${HDF5_SOURCE_DIR}/bin/batch/${LOCAL_BATCH_SCRIPT_COMMAND}
        ${HDF5_BINARY_DIR}/${LOCAL_BATCH_SCRIPT_COMMAND} ESCAPE_QUOTES @ONLY
    )
  endif ()
  if (LOCAL_BATCH_SCRIPT_NAME)
    configure_file (
        ${HDF5_SOURCE_DIR}/bin/batch/${LOCAL_BATCH_SCRIPT_NAME}.in.cmake
        ${HDF5_BINARY_DIR}/${LOCAL_BATCH_SCRIPT_NAME} ESCAPE_QUOTES @ONLY
    )
  endif ()
  if (LOCAL_BATCH_SCRIPT_PARALLEL_NAME)
    configure_file (
        ${HDF5_SOURCE_DIR}/bin/batch/${LOCAL_BATCH_SCRIPT_PARALLEL_NAME}.in.cmake
        ${HDF5_BINARY_DIR}/${LOCAL_BATCH_SCRIPT_PARALLEL_NAME} ESCAPE_QUOTES @ONLY
    )
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Set up library builds
#-----------------------------------------------------------------------------
if (BUILD_STATIC_LIBS)
  add_library (${HDF5_LIB_TARGET} STATIC ${common_SRCS} ${HDF5_SRC_BINARY_DIR}/H5build_settings.c ${H5_PUBLIC_HEADERS} ${H5_PRIVATE_HEADERS} ${H5_GENERATED_HEADERS} ${H5_MODULE_HEADERS})
  target_include_directories (${HDF5_LIB_TARGET}
      PRIVATE "${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR};${HDF5_COMP_INCLUDE_DIRECTORIES};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>;$<BUILD_INTERFACE:${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR}>"
  )
  target_compile_options(${HDF5_LIB_TARGET} PRIVATE "${HDF5_CMAKE_C_FLAGS}")
  target_compile_definitions(${HDF5_LIB_TARGET}
      PUBLIC
          ${HDF_EXTRA_C_FLAGS}
      PRIVATE
          "$<$<BOOL:${HDF5_ENABLE_TRACE}>:H5_DEBUG_API>" # Enable tracing of the API
          "$<$<BOOL:${HDF5_ENABLE_DEBUG_APIS}>:${HDF5_DEBUG_APIS}>"
          "$<$<CONFIG:Developer>:${HDF5_DEVELOPER_DEFS}>"
  )
  TARGET_C_PROPERTIES (${HDF5_LIB_TARGET} STATIC)
  target_link_libraries (${HDF5_LIB_TARGET}
      PRIVATE ${LINK_LIBS} ${LINK_COMP_LIBS}
      PUBLIC ${LINK_PUB_LIBS}
  )
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT ${HDF5_LIB_TARGET})
  H5_SET_LIB_OPTIONS (${HDF5_LIB_TARGET} ${HDF5_LIB_NAME} STATIC 0)
  set_target_properties (${HDF5_LIB_TARGET} PROPERTIES FOLDER libraries)

  set (install_targets ${HDF5_LIB_TARGET})
endif ()

if (BUILD_SHARED_LIBS)
  add_library (${HDF5_LIBSH_TARGET} SHARED ${common_SRCS} ${HDF5_SRC_BINARY_DIR}/H5build_settings.c ${H5_PUBLIC_HEADERS} ${H5_PRIVATE_HEADERS} ${H5_GENERATED_HEADERS} ${H5_MODULE_HEADERS})
  target_include_directories (${HDF5_LIBSH_TARGET}
      PRIVATE "${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR};${HDF5_COMP_INCLUDE_DIRECTORIES};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      PUBLIC "$<$<BOOL:${HDF5_ENABLE_HDFS}>:${HDFS_INCLUDE_DIR}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>;$<BUILD_INTERFACE:${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR}>"
  )
  target_compile_options(${HDF5_LIBSH_TARGET} PRIVATE "${HDF5_CMAKE_C_FLAGS}")
  target_compile_definitions(${HDF5_LIBSH_TARGET}
      PUBLIC
          "H5_BUILT_AS_DYNAMIC_LIB"
          ${HDF_EXTRA_C_FLAGS}
      PRIVATE
          "$<$<BOOL:${HDF5_ENABLE_TRACE}>:H5_DEBUG_API>"  # Enable tracing of the API
          "$<$<BOOL:${HDF5_ENABLE_DEBUG_APIS}>:${HDF5_DEBUG_APIS}>"
          "$<$<CONFIG:Developer>:${HDF5_DEVELOPER_DEFS}>"
  )
  TARGET_C_PROPERTIES (${HDF5_LIBSH_TARGET} SHARED)
  target_link_libraries (${HDF5_LIBSH_TARGET}
      PRIVATE ${LINK_LIBS} ${LINK_COMP_LIBS}
      PUBLIC ${LINK_PUB_LIBS}
  )
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_LIBSH_TARGET}")
  H5_SET_LIB_OPTIONS (${HDF5_LIBSH_TARGET} ${HDF5_LIB_NAME} SHARED "LIB")
  set_target_properties (${HDF5_LIBSH_TARGET} PROPERTIES FOLDER libraries)

  set (install_targets ${install_targets} ${HDF5_LIBSH_TARGET})
endif ()

#-----------------------------------------------------------------------------
# Add Target to clang-format
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_FORMATTERS)
  if (BUILD_STATIC_LIBS)
    clang_format (HDF5_SRC_FORMAT ${HDF5_LIB_TARGET})
  else ()
    clang_format (HDF5_SRC_FORMAT ${HDF5_LIBSH_TARGET})
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
if (NOT HDF5_INSTALL_NO_DEVELOPMENT)
  install (
      FILES
          ${H5_PUBLIC_HEADERS}
          ${H5_PUBLIC_GENERATED_HEADERS}
          ${HDF5_SRC_BINARY_DIR}/H5pubconf.h
      DESTINATION
          ${HDF5_INSTALL_INCLUDE_DIR}
      COMPONENT
          headers
  )
endif ()

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_LIBSH_TARGET} ${HDF5_INSTALL_BIN_DIR} libraries)
  endif ()
  if (BUILD_STATIC_LIBS)
    INSTALL_TARGET_PDB (${HDF5_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} libraries)
  endif ()

  install (
      TARGETS
          ${install_targets}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT libraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT libraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT libraries
      FRAMEWORK DESTINATION ${HDF5_INSTALL_FWRK_DIR} COMPONENT libraries
      INCLUDES DESTINATION include
  )
endif ()

#-----------------------------------------------------------------------------
# Create pkgconfig files
#-----------------------------------------------------------------------------
set (_PKG_CONFIG_PREFIX ${CMAKE_INSTALL_PREFIX})
set (_PKG_CONFIG_EXEC_PREFIX \${prefix})
set (_PKG_CONFIG_LIBDIR \${exec_prefix}/${HDF5_INSTALL_LIB_DIR})
set (_PKG_CONFIG_INCLUDEDIR \${prefix}/${HDF5_INSTALL_INCLUDE_DIR})
set (_PKG_CONFIG_LIBNAME "${HDF5_LIB_NAME}")
set (_PKG_CONFIG_VERSION "${HDF5_PACKAGE_VERSION}")
set (PKGCONFIG_LIBNAME "${HDF5_LIB_NAME}")
if (${HDF_CFG_NAME} MATCHES "Debug" OR ${HDF_CFG_NAME} MATCHES "Developer")
  set (PKGCONFIG_LIBNAME "${PKGCONFIG_LIBNAME}${CMAKE_DEBUG_POSTFIX}")
endif ()

#foreach (libs ${LINK_LIBS})
#  set (_PKG_CONFIG_LIBS_PRIVATE "${_PKG_CONFIG_LIBS_PRIVATE} -l${libs}")
#endforeach ()

# The settings for the compression libs depends on if they have pkconfig support
# Assuming they don't
foreach (libs ${LINK_COMP_LIBS})
#  set (_PKG_CONFIG_REQUIRES_PRIVATE "${_PKG_CONFIG_REQUIRES_PRIVATE} -l${libs}")
  get_target_property (libname ${libs} OUTPUT_NAME)
  set (_PKG_CONFIG_LIBS_PRIVATE "${_PKG_CONFIG_LIBS_PRIVATE} -l${libname}")
endforeach ()

#if (BUILD_STATIC_LIBS)
#  set (_PKG_CONFIG_LIBS "${_PKG_CONFIG_LIBS} -l${PKGCONFIG_LIBNAME}")
#endif ()
if (BUILD_SHARED_LIBS)
  set (_PKG_CONFIG_SH_LIBS "${_PKG_CONFIG_SH_LIBS} -l${PKGCONFIG_LIBNAME}")
endif ()

set (_PKG_CONFIG_REQUIRES)

configure_file (
    ${HDF_CONFIG_DIR}/libhdf5.pc.in
    ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_LIB_CORENAME}.pc
    @ONLY
)
install (
    FILES ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_LIB_CORENAME}.pc
    DESTINATION ${HDF5_INSTALL_LIB_DIR}/pkgconfig
    COMPONENT libraries
)

if (H5_HAVE_PKGCONFIG AND NOT HDF5_BUILD_HL_LIB)
  set (_PKG_CONFIG_COMPILER ${HDF5_H5CC_C_COMPILER})
  configure_file (
      ${HDF_CONFIG_DIR}/libh5cc.in
      ${HDF5_BINARY_DIR}/CMakeFiles/h5cc
      @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/h5cc
      DESTINATION ${HDF5_INSTALL_BIN_DIR}
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      COMPONENT libraries
  )
  if (HDF5_ENABLE_PARALLEL AND MPI_C_FOUND)
    #legacy requires a different name
    configure_file (
        ${HDF_CONFIG_DIR}/libh5cc.in
        ${HDF5_BINARY_DIR}/CMakeFiles/h5pcc
        @ONLY
    )
    install (
        FILES ${HDF5_BINARY_DIR}/CMakeFiles/h5pcc
        DESTINATION ${HDF5_INSTALL_BIN_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT libraries
    )
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build documentation
#-----------------------------------------------------------------------------
if (DOXYGEN_FOUND)
  # This custom target and doxygen/configure work together
  # Replace variables inside @@ with the current values
  add_custom_target (hdf5lib_doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${HDF5_BINARY_DIR}/Doxyfile
        DEPENDS ${HDF5_SRC_BINARY_DIR}/H5build_settings.c
        WORKING_DIRECTORY ${HDF5_SRC_DIR}
        COMMENT "Generating HDF5 library Source API documentation with Doxygen"
        VERBATIM )

  if (NOT TARGET doxygen)
    add_custom_target (doxygen)
  endif ()

  add_dependencies (doxygen hdf5lib_doc)

endif ()
