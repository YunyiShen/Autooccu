source("./R/misc.R")
#source("Moller_island.R")
source("./R/Main_Sampler.R")
require(Matrix)
require(Rcpp)
Rcpp::sourceCpp("src/IsingCpp_CFTP_sparse.cpp")

###### graph data ######
link = "./data/TJH/"
island = read.csv(paste0(link,"TJH_unique_grids.csv"))

link_outer = as.matrix( read.csv(paste0(link, "link_TJH.csv")))
#link_inner = as.matrix( read.csv("link_outer.csv",row.names = 1))
link_inner = 0 * link_outer # this makes it a mainland-island system
link_mainland = matrix(0,97,1)

distM_full = as.matrix( read.csv(paste0(link,"distM_TJH.csv")))
distM_mainland = matrix(100,97,1)

intcd = min(min((link_outer*distM_full)[(link_outer*distM_full)>0]))
normd = max(max(link_outer*distM_full))-intcd


distM_full = (distM_full-intcd)/normd # normalizing the distance
distM_mainland = (distM_mainland-intcd)/normd

detmat = list(as.matrix(read.csv(paste0(link,"BM_GZL_HHD_ZH_20dayfull.csv")))[-(1:97+(97*3)),])
#full = read.csv(paste0(link,"PA_all_full.csv"),row.names=1)
#Z_sample = matrix(c(full$Coyote,full$Fox_red,full$Bobcat))

###### simulation ######

spp_mat = matrix(1,3,3)
diag(spp_mat)=0
envX = cbind(matrix(1,97,1),island$ELE)
envX = apply(envX,2,function(k){(k-mean(k))/(sd(k))})
envX[,1]=1

theta = list(beta_occu = rep(0,6),
             beta_det = rep(0,6),
             eta_intra = c(0,0,0),
             eta_inter = c(.1,.1,.1),
             #d_inter = c(.2,.2),
             spp_mat = 0.1 * spp_mat,
             spp_mat_det = -0.1 * spp_mat)

link_map = 
  list(inter = 0*link_outer * exp(-distM_full),
       intra = link_inner)

nrep = 1
nspp = 3
  #    then in the second level list, it is a matrix with nrow = site ncol = ncov, 

#Pdet = Pdet_multi(nperiod, envX,detX[[1]], theta$beta_det, nspp=nrow(spp_mat))

#detmat = Sample_detection(nrep,nperiod,envX,detX,theta$beta_det,nspp = nrow(spp_mat),Z=Z_sample)
#detmat = lapply(detmat,function(w){w*2-1}) 

#sppmat_det = -0.1 * spp_mat
#Pdet_Ising(nperiod,envX,detX[[1]],beta_det = theta$beta_det,theta$sppmat_det,Z = Z_sample,detmat[[1]])
#Pdet_Ising_rep(1,27,envX,NULL,1:6/10,theta$spp_mat_det,Z = Z_absolute,detmat)

#no_obs=150:155
#no_obs = c(no_obs, no_obs + 155, no_obs + 310)

nspp = 3

vars_prop = list( beta_occu = rep(8e-3,nspp * ncol(envX))
                  ,beta_det = rep(5e-3,nspp * ( ncol(envX)) ) # no extra det thing
                  ,eta_intra = rep(5e-3,nspp)
                  ,eta_inter = rep(5e-3,nspp)
                  #,d_intra=rep(2.5e-5,nspp)
                  #,d_inter = rep(1e-4,nspp)
                  ,spp_mat = 5e-3
                  ,spp_mat_det = 5e-3)

para_prior = list( beta_occu = rep(1000,nspp * ncol(envX))
                   ,beta_det = rep(.1,1000,.1,1000,.1,1000 )
                   ,eta_intra = rep(1000,nspp)
                   ,eta_inter = rep(1000,nspp)
                   ,d_intra=rep(1000,nspp)
                   ,d_inter = rep(1000,nspp)
                   ,spp_mat = 1
                   ,spp_mat_det = 1)

detmat_nona = lapply(detmat,function(mat){
  mat[is.na(mat)]=-1
  return(mat)
})
Z_absolute = (sapply(detmat_nona,function(detmat_i){rowSums((detmat_i+1)/2)>0})) * 2 - 1

rm(detmat_nona)
datatemp  = data.frame(island,
                       Z1 = Z_absolute[1:97,],
                       Z2 = Z_absolute[1:97+97],
                       Z3 = Z_absolute[1:97+2*97]
                       )

#Z_absolute = (sapply(detmat,function(detmat_i){rowSums((detmat_i+1)/2)>0})) * 2 - 1
require(ggplot2)
ggplot(data = datatemp,aes(x=LONG,y=LAT,color = Z2))+
  geom_point()


kk = IsingOccu.fit.Murray.sampler_Ising_det(X = envX, detmat =  detmat
                                            , detX =  NULL
                                            , mcmc.iter = 100000, burn.in = 10000
                                            , vars_prop = vars_prop
                                            , para_prior = para_prior
                                            
                                            , uni_prior = F
                                            , distM=distM_full,link_map=link_map
                                            , dist_mainland =  distM_mainland , link_mainland =  link_mainland * exp(-2*distM_mainland)
                                            , int_range_intra="nn",int_range_inter="nn"
                                            
                                            , seed = 42
                                            , ini = theta,thin.by = 10,report.by = 5000,nIter = 50,Gibbs = T)



